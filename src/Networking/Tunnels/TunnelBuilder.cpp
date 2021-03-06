/*!
* @file
* @brief TunnelBuilder
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 7/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TunnelBuilder.h"
#include "Networking/Tunnels/EthTap.h"
#include "Networking/Tunnels/RawSocket.h"
#include "Networking/Tunnels/TCPServerTunnel.h"
#include "Networking/Tunnels/TCPTunnel.h"
#include "Networking/Tunnels/UDPTunnel.h"
#include "Algorithms/Logging/Logger.h"
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "QKDInterfaces/Tunnels.pb.h"

#include <cryptopp/osrng.h>
#if defined(CRYPTOPP_HASRDRAND)
    #include <cryptopp/rdrand.h>
#endif
#include <cryptopp/cpu.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/rng.h>
#include <cryptopp/secblock.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include "QKDInterfaces/Tunnels.pb.h"
using cqp::remote::tunnels::TunnelEndDetails;

#include "KeyManagement/KeyStores/IKeyStore.h"
#include "Networking/Tunnels/DeviceIO.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include <cryptopp/rng.h>
#include <cryptopp/seckey.h>
#include <cryptopp/rsa.h>
#include <cryptopp/oids.h>
#include <cryptopp/eccrypto.h>
// ASN1 is a namespace, not an object
#include <cryptopp/asn.h>
#include <cryptopp/filters.h>

#include <grpc++/server_builder.h>
#include "Algorithms/Datatypes/Units.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Net/DNS.h"

#include <thread>
using grpc::Status;
using grpc::StatusCode;
using google::protobuf::Empty;
using grpc::ServerContext;

namespace cqp
{
    namespace tunnels
    {
        const unsigned long rawInputBufferSize = 2_MiB;

        TunnelBuilder::~TunnelBuilder()
        {
            keepGoing = false;
            if(encodeThread.joinable())
            {
                encodeThread.join();
            }
        }

        TunnelBuilder::TunnelBuilder(const remote::tunnels::CryptoScheme& crypto,
                                     std::shared_ptr<grpc::ChannelCredentials> clientCreds) :
            clientCreds{clientCreds}
        {
            using namespace CryptoPP;
            LOGDEBUG("Cipher Mode=" + crypto.mode() + ", SubMode=" + crypto.submode() + ", BlockCypher=" + crypto.blockcypher());

            // create the process which will encrypt/decrypt data
            if(crypto.mode() == Modes::GCM && crypto.blockcypher() == BlockCiphers::AES)
            {
                if(crypto.submode() == SubModes::Tables2K)
                {
                    encryptorCypher.reset(new GCM<AES, GCM_2K_Tables>::Encryption());
                    decryptorCypher.reset(new GCM<AES, GCM_2K_Tables>::Decryption());
                }
                else if(crypto.submode() == SubModes::Tables64K)
                {
                    encryptorCypher.reset(new GCM<AES, GCM_64K_Tables>::Encryption());
                    decryptorCypher.reset(new GCM<AES, GCM_64K_Tables>::Decryption());
                }
            }

            if(encryptorCypher)
            {
                //  client <--> crypto <--> Serialiser <-> dataChannel
                //               ^
                //      keyGen --'

                // the encryption filter uses the encryptorCypher to encrypt the data and put it in the encryptorOutput

                try
                {
                    encryptor.reset(new AuthenticatedEncryptionFilter(*encryptorCypher, nullptr, /*putAAD*/ false, AES::BLOCKSIZE));
                }
                catch (const Exception& e)
                {
                    LOGERROR(e.what());
                }
            }
            else
            {
                // no encryption
                LOGERROR("No valid encryption selected");
            }

            if(decryptorCypher)
            {
                decryptor.reset(new AuthenticatedDecryptionFilter(
                                    *decryptorCypher,
                                    nullptr, // attach will be called when the client is created
                                    AuthenticatedDecryptionFilter::MAC_AT_END, AES::BLOCKSIZE
                                ));
            }
            using namespace CryptoPP;

            rng = new AutoSeededX917RNG<AES>();

            if(rng == nullptr)
            {
                LOGWARN("Falling back to AutoSeededRandomPool");
                rng = new AutoSeededRandomPool();
            }

        }

        TunnelBuilder::TunnelBuilder(const remote::tunnels::CryptoScheme& crypto,
                                     const std::string& transferListenAddress,
                                     std::shared_ptr<grpc::ServerCredentials> creds,
                                     std::shared_ptr<grpc::ChannelCredentials> clientCreds) :
            TunnelBuilder(crypto, clientCreds)
        {
            // create the server
            grpc::ServerBuilder builder;
            // grpc will create worker threads as it needs, idle work threads
            // will be stopped if there are more than this number running
            // setting this too low causes large number of thread creation+deletions, default = 2
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 50);

            auto realListenAddress = transferListenAddress;
            if(realListenAddress.empty())
            {
                realListenAddress = std::string(net::AnyAddress) + ":0";
            }

            builder.AddListeningPort(realListenAddress, creds, &transferListenPort);

            LOGTRACE("Registering services");
            // Register services
            builder.RegisterService(this);
            // ^^^ Add new services here ^^^ //

            LOGTRACE("Starting server on " + realListenAddress);
            server = builder.BuildAndStart();

            URI address(realListenAddress);
            transferListenHost = net::GetHostname();

            if(!server)
            {
                LOGERROR("Failed to start encrypted channel server");
            }
            else
            {
                LOGDEBUG("Server ready on " + GetListenAddress());
            }
        } // TunnelBuilder

        Status TunnelBuilder::ConfigureEndpoint(const remote::tunnels::TunnelEndDetails& details,
                                                std::shared_ptr<grpc::Channel> keyFactoryChannel,
                                                const std::string& keyStoreTo,
                                                const remote::tunnels::KeyLifespan& keyLifespan)
        {
            Status result;

            LOGDEBUG("Endpoint details:\n" +
                     "   Unencrypted port: " + details.clientdataporturi() + "\n" +
                     "   Far KeyStore: " + keyStoreTo + "\n"
                    );

            // wait for any existing setup to come down
            keepGoing = false;
            if(encodeThread.joinable())
            {
                encodeThread.join();
            }

            if(keyStoreTo.empty())
            {
                result = Status(StatusCode::INVALID_ARGUMENT, "Bad endpoint parameters specified");
            }
            else
            {
                // store the new values for creating the key
                currentKeyStoreTo = keyStoreTo;
                currentKeyLifespan = keyLifespan;
                // channel for getting keys
                myKeyFactoryChannel = keyFactoryChannel;

                // create the device which will read/write unencrypted data
                client = UriToTunnel(details.clientdataporturi());

                if(client && encryptor && decryptor)
                {
                    keepGoing = true;
                    result = Status();
                }
                else
                {
                    LOGERROR("Setup failed");
                    result = Status(StatusCode::INTERNAL, "Setup failed");
                } // if(client && encryptor && decryptor)
            } // else

            return result;
        } // ConfigureEndpoint

        void TunnelBuilder::StartTransfer(const std::string& farSide)
        {
            LOGTRACE("");
            if(client && encryptor && decryptor)
            {
                decryptor->Attach(new CryptoPP::Redirector(*client));
                keepGoing = true;
                encodeThread = std::thread(&TunnelBuilder::EncodingWorker, this, farSide);
            }
        }

        void TunnelBuilder::Shutdown()
        {
            keepGoing = false;
            if(encodeThread.joinable())
            {
                encodeThread.join();
            }
        } // Shutdown

        Status TunnelBuilder::ReadEncrypted(::grpc::internal::ReaderInterface<remote::EncryptedDataValues>* stream,
                                            remote::IKey::Stub* keyFactory)
        {
            LOGTRACE("Starting");
            using namespace CryptoPP;
            Status result;
            remote::SharedKey sharedKey;
            remote::EncryptedDataValues incomming;
            // read encrypted data from the stream
            while(result.ok() && stream->Read(&incomming) && client && keepGoing)
            {
                if(incomming.keyid() != sharedKey.keyid())
                {
                    LOGDEBUG("Getting key: " + std::to_string(incomming.keyid()));
                    sharedKey.Clear();
                    grpc::ClientContext keyGenContext;
                    remote::KeyRequest request;
                    request.set_siteto(currentKeyStoreTo);
                    request.set_keyid(incomming.keyid());
                    sharedKey.Clear();
                    // get the for this data block
                    if(!LogStatus(keyFactory->GetSharedKey(&keyGenContext, request, &sharedKey)).ok())
                    {
                        LOGERROR("Failed to get key " + std::to_string(incomming.keyid()));
                    }
                }

                if(decryptorCypher->IsValidKeyLength(sharedKey.keyvalue().size()))
                {
                    using std::chrono::high_resolution_clock;
                    high_resolution_clock::time_point timerStart = high_resolution_clock::now();

                    decryptorCypher->SetKeyWithIV(
                        reinterpret_cast<const unsigned char*>(sharedKey.keyvalue().data()),
                        sharedKey.keyvalue().size(),
                        reinterpret_cast<const unsigned char*>(incomming.iv().data()),
                        incomming.iv().size());

                    try
                    {
                        // send data to the input filter
                        StringSource decryptorInput(incomming.payload(), true, new Redirector(*decryptor));
                        // because of attachment, the data flows to the decryptor and out to the client
                        // see ConfigureEndpoint for pipeline setup
                    }
                    catch(const std::exception& e)
                    {
                        LOGERROR(e.what());
                        result = Status(StatusCode::DATA_LOSS, "Decryption failed", e.what());
                    }

                    stats.decryptTime.Update(high_resolution_clock::now() - timerStart);
                }
                else
                {
                    result = LogStatus(Status(StatusCode::INVALID_ARGUMENT, "Invalid Key"));
                }

            }
            LOGTRACE("Ending");
            return result;
        }

        Status TunnelBuilder::Transfer(ServerContext* ctx, ::grpc::ServerReaderWriter<remote::EncryptedDataValues, remote::EncryptedDataValues>* stream)
        {
            Status result;
            using namespace CryptoPP;
            // stub for getting existing keys
            auto keyFactory = remote::IKey::NewStub(myKeyFactoryChannel);

            while(!client || !client->WaitUntilReady(std::chrono::milliseconds(1000)))
            {
                LOGINFO("Waiting for client data channel");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            decryptor->Attach(new CryptoPP::Redirector(*client));
            keepGoing = true;

            // start reading encrypted data
            auto reader = std::async(std::launch::async, [&]()
            {
                return LogStatus(ReadEncrypted(stream, keyFactory.get()));
            });

            // encrypt data and send it
            result = LogStatus(WriteEncrypted(stream, keyFactory.get()));
            ctx->TryCancel();
            keepGoing = false;
            LOGTRACE("Waiting for reader to finish");
            // wait for reader to stop
            reader.wait();

            LOGDEBUG("Decryptor finished");

            return Status();
        } // Transfer


        std::string TunnelBuilder::GetListenAddress() const
        {
            return transferListenHost + ":" + std::to_string(transferListenPort);
        }

        DeviceIO* TunnelBuilder::UriToTunnel(const URI& portUri)
        {
            DeviceIO* client = nullptr;
            // handle the type of client port to create
            switch (str2int(portUri.GetScheme().c_str()))
            {
            case str2int(DeviceTypes::tap):
            case str2int(DeviceTypes::tun):
                // raw Ethernet/ip packets
                client = EthTap::Create(portUri);
                break;

            case str2int(DeviceTypes::eth):
                client = RawSocket::Create(portUri);
                break;

            case str2int(DeviceTypes::udp):
                client = new UDPTunnel(portUri);
                break;
            case str2int(DeviceTypes::tcp):
                client = new TCPTunnel(portUri);
                break;

            case str2int(DeviceTypes::tcpsrv):
                client = new TCPServerTunnel(portUri);
                break;
            default:
                LOGERROR("Unsupported scheme: " + portUri.GetScheme());
            }

            return client;
        } // UriToTunnel

        bool TunnelBuilder::ChangeKey(const remote::tunnels::KeyLifespan& keyLifeSpan, uint64_t bytesUsedOnKey,
                                      const std::chrono::high_resolution_clock::time_point& timeKeyGenerated)
        {
            using std::chrono::high_resolution_clock;
            using namespace std::chrono;
            using remote::Duration;

            bool result = keyLifeSpan.maxbytes() > 0 && bytesUsedOnKey >= keyLifeSpan.maxbytes();

            // only check for time elapsed if the key has been used for some bytes
            if(!result && bytesUsedOnKey > 0)
            {
                auto keyDuration = high_resolution_clock::now() - timeKeyGenerated;

                switch (keyLifeSpan.maxage().scale_case())
                {
                case Duration::ScaleCase::kSeconds:
                    result = keyDuration >= seconds(keyLifeSpan.maxage().seconds());
                    break;
                case Duration::ScaleCase::kMilliseconds:
                    result = keyDuration >= milliseconds(keyLifeSpan.maxage().seconds());
                    break;
                case Duration::ScaleCase::SCALE_NOT_SET:
                    break;
                }
            }
            return result;
        }

        Status TunnelBuilder::WriteEncrypted(::grpc::internal::WriterInterface<remote::EncryptedDataValues>* stream,
                                             remote::IKey::Stub* keyFactory)
        {
            LOGTRACE("Starting");
            using namespace std;
            using namespace std::chrono;
            using namespace CryptoPP;
            using chrono::high_resolution_clock;

            Status result;
            remote::SharedKey sharedKey;
            uint64_t bytesUsedOnKey = 0;
            high_resolution_clock::time_point timeKeyGenerated;
            SecByteBlock buffer(rawInputBufferSize);
            std::string payload;
            encryptor->Attach(new StringSink(payload));

            while(keyFactory && keepGoing && stream)
            {

                try
                {
                    // see if we need to get a new key
                    if(sharedKey.keyvalue().empty() || ChangeKey(currentKeyLifespan, bytesUsedOnKey, timeKeyGenerated))
                    {
                        using std::chrono::high_resolution_clock;
                        high_resolution_clock::time_point timerStart = high_resolution_clock::now();

                        sharedKey.Clear();

                        LOGDEBUG("Getting new shared key");

                        // try and get a key
                        do
                        {
                            grpc::ClientContext keyGenContext;
                            remote::KeyRequest request;
                            request.set_siteto(currentKeyStoreTo);
                            // if the Get succeeds, update the trigger values
                            if(LogStatus(keyFactory->GetSharedKey(&keyGenContext, request, &sharedKey)).ok())
                            {
                                bytesUsedOnKey = 0;
                                timeKeyGenerated = high_resolution_clock::now();
                                stats.keyChangeTime.Update(high_resolution_clock::now() - timerStart);
                            }
                            else
                            {
                                LOGWARN("Failed to get a key, retrying...");
                            }
                            // if it failed and we don't have a key at all: try again
                        }
                        while(sharedKey.keyvalue().empty() && keepGoing);
                        // if it failed but we have a key already,
                        //  keep processing incomming data and try again on the next message

                    } // if

                    if(encryptorCypher->IsValidKeyLength(sharedKey.keyvalue().size()))
                    {
                        size_t numRead = 0;
                        if(!client->Read(buffer.begin(), buffer.SizeInBytes(), numRead))
                        {
                            LOGERROR("Client Socket closed.");
                            keepGoing = false;
                        }

                        if(numRead > 0)
                        {
                            using std::chrono::high_resolution_clock;
                            high_resolution_clock::time_point timerStart = high_resolution_clock::now();

                            SecByteBlock iv(AES::BLOCKSIZE);
                            rng->GenerateBlock(iv, iv.size());

                            encryptorCypher->SetKeyWithIV(
                                reinterpret_cast<const unsigned char*>(sharedKey.keyvalue().data()),
                                sharedKey.keyvalue().size(),
                                iv,
                                iv.size());

                            remote::EncryptedDataValues messageData;
                            // build data to send to the other side
                            messageData.set_keyid(sharedKey.keyid());
                            messageData.mutable_iv()->assign(reinterpret_cast<const char*>(iv.data()), iv.size());

                            // push data into the encryptor
                            encryptor->Put2(buffer.data(), numRead, -1, true);
                            // because of earlier attachment, the encryptor output will be put into the message payload
                            messageData.set_payload(payload);

                            // send for decryption at the other side
                            if(!stream->Write(messageData))
                            {
                                LOGERROR("Failed to send encrypted message");
                                keepGoing = false;
                            }

                            // track the number of bytes this key has encrypted
                            bytesUsedOnKey += numRead;

                            payload.clear();
                            buffer.CleanNew(buffer.size());

                            stats.encryptTime.Update(high_resolution_clock::now() - timerStart);
                            stats.bytesEncrypted.Update(numRead);
                        }

                    }
                    else
                    {
                        LOGERROR("Invalid Key size:" + std::to_string(sharedKey.keyvalue().size()));
                        keepGoing = false;
                    } // else
                } // try
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                } // catch

            } // while(keepGoing)

            LOGTRACE("Ending");
            return result;
        }

        void TunnelBuilder::EncodingWorker(std::string farSide)
        {
            Status result;
            auto keyFactory = remote::IKey::NewStub(myKeyFactoryChannel);
            // channel for transferring the encrypted data
            // communication with peer
            LOGDEBUG("Connecting to encrypted channel " + farSide);
            auto farSideEP = grpc::CreateChannel(farSide, clientCreds);

            if(farSideEP)
            {
                auto farSide = remote::ITransfer::NewStub(farSideEP);

                // create a pipeline which will react to data from the client and pass it to the encryptor

                while(!client->WaitUntilReady())
                {
                    LOGINFO("Waiting for client");
                }

                grpc::ClientContext transferContext;
                auto farSideStream = farSide->Transfer(&transferContext);

                // launch a thread to decrypt the data
                auto reader = std::async(std::launch::async, [&]()
                {
                    return LogStatus(ReadEncrypted(farSideStream.get(), keyFactory.get()));
                });

                // read data an encrypt it
                result = LogStatus(WriteEncrypted(farSideStream.get(), keyFactory.get()));

                transferContext.TryCancel();
                // encryptor has finished, wait for reader to finish
                if(farSideStream)
                {
                    farSideStream->WritesDone();
                }
                reader.wait();

                if(farSideStream)
                {
                    LogStatus(farSideStream->Finish());
                }
            }
            else
            {
                LOGERROR("Failed to connect to far side");
            }

            delete client;
            client = nullptr;
            keepGoing = false;

            LOGDEBUG("Encryptor finished");
        } // EncodingWorker
    } // namespace tunnels
} // namespace cqp
