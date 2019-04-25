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
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"

#include "Algorithms/Datatypes/URI.h"
#include "QKDInterfaces/ITransfer.grpc.pb.h"
#include "QKDInterfaces/Tunnels.pb.h"
#include "Algorithms/Datatypes/Units.h"
#include "Networking/Tunnels/Stats.h"
#include <cryptopp/secblock.h>
#include <cryptopp/filters.h>
#include <thread>
#include <grpc++/security/server_credentials.h>
#include "Algorithms/Util/Strings.h"

namespace grpc
{
    class ChannelCredentials;
    class ServerCredentials;
}

namespace cqp
{
    namespace tunnels
    {
        class DeviceIO;

        namespace Modes
        {
            /// No encryption mode
            CONSTSTRING None = "None";
            /// GCM+AES
            CONSTSTRING GCM = "GCM";
        }

        namespace SubModes
        {
            /// no submod
            CONSTSTRING None = "None";
            /// GCM with 2K tables
            CONSTSTRING Tables2K = "Tables2K";
            /// GCM with 64K tables
            CONSTSTRING Tables64K = "Tables64K";
        }

        namespace BlockCiphers
        {
            /// no cipher
            CONSTSTRING None = "None";
            /// AES cipher
            CONSTSTRING AES = "AES";
        }

        /// bits per key
        enum KeySizes : uint8_t
        {
            Key128 = 16,
            Key256 = 32
        };

        namespace RandomNumberGenerators
        {
            /// use any source of randomness
            CONSTSTRING Any = "Any";
            /// use the OSX917 algorithm
            CONSTSTRING OSX917 = "OSX917";
            /// use hardware rng
            CONSTSTRING RDRAND = "RDRAND";
            /// use any software rng
            CONSTSTRING SWRNG = "SWRNG";
        }
        /// string constants for devices types
        namespace DeviceTypes
        {
            /// ethernet device
            CONSTSTRING eth = "eth";
            /// tunnel device
            /// @details https://en.wikipedia.org/wiki/TUN/TAP
            CONSTSTRING tun = "tun";   // raw IP packets
            /// tap device
            /// @details https://en.wikipedia.org/wiki/TUN/TAP
            CONSTSTRING tap = "tap";   // raw ethernet packets
            /// tcp socket connected to a server
            CONSTSTRING tcp = "tcp";
            /// tcp server socket for recieving connections
            CONSTSTRING tcpsrv = "tcpsrv";
            /// udp socket for fire and forget
            CONSTSTRING udp = "udp";
            /// clavis 2 QKD device
            CONSTSTRING clavis2 = "clavis2";
        }
        /**
         * @brief The TunnelBuilder class
         * Constructs the sockets needed to transfer the data
         */
        class NETWORKING_EXPORT TunnelBuilder : protected remote::ITransfer::Service
        {
        public:

            /**
             * @brief TunnelBuilder
             * Constructor
             * @param crypto The kind of encryption to use
             * @param[in,out] transferListenPort The port number to use for encrypted data transfer
             * @param creds The server credentials to use
             * @param clientCreds The credentials to use when connecting to peer
             */
            TunnelBuilder(const remote::tunnels::CryptoScheme& crypto, int& transferListenPort, std::shared_ptr<grpc::ServerCredentials> creds,
                          std::shared_ptr<grpc::ChannelCredentials> clientCreds);

            /**
             * @brief ConfigureEndpoint
             * Construct the tunnel by starting an endpoint
             * @param details
             * @param keyFactoryChannel
             * @param keyStoreTo
             * @param keyLifespan
             * @return true on success
             */
            grpc::Status ConfigureEndpoint(const remote::tunnels::TunnelEndDetails& details,
                                           std::shared_ptr<grpc::Channel> keyFactoryChannel,
                                           const std::string& keyStoreTo,
                                           const remote::tunnels::KeyLifespan& keyLifespan);

            /**
             * @brief Shutdown
             * Stop the tunnel
             */
            void Shutdown();

            ///@{
            /// ITransfer interface

            /**
             * @brief Transfer
             * Send encrypted data to/from a server
             * @param context
             * @param reader
             * @param response
             * @return status
             * @details
             * @startuml DecodingBehaviour
                hide footbox

                title "Decoding"

                participant TunnelBuilder
                participant KeyFactory
                participant decryptorCypher
                participant client

                [-> TunnelBuilder : Transfer
                    activate TunnelBuilder

                    TunnelBuilder -> client : WaitUntilReady()

                    loop

                        TunnelBuilder -> reader : Read()
                        note over TunnelBuilder
                            Read a message from the stream
                        end note
                        alt the key has changed
                            TunnelBuilder -> KeyFactory : GetSharedKey(keyId)
                        end alt
                        TunnelBuilder -> decryptorCypher : SetKeyWithIV
                        TunnelBuilder -> decryptorCypher : Decrypt(payload)
                        TunnelBuilder -> client : Send

                    end loop

                @enduml
             */
            grpc::Status Transfer(grpc::ServerContext* context, ::grpc::ServerReader<remote::EncryptedDataValues>* reader, google::protobuf::Empty* response) override;

            ///@}

            /// Destructor
            ~TunnelBuilder() override;

            /// stats created my this class
            Statistics stats;
        protected:
            /**
             * @brief UriToTunnel
             * Construct a suitable io device for the url specified
             * @param portUri
             * @return true on success
             */
            static DeviceIO* UriToTunnel(const URI& portUri);

            /**
             * @brief EncodingWorker
             * Process incoming data from the client and encrypt it, passing it to the other end
             * @details
             * @startuml EncodingWorkerBehaviour
                hide footbox

                title "EncodingWorker"

                participant TunnelBuilder
                participant KeyFactory
                participant rng
                participant encryptorCypher
                participant client
                boundary ITransfer

                [-> TunnelBuilder : EncodingWorker
                    activate TunnelBuilder
                    TunnelBuilder -> client : WaitUntilReady()
                    loop
                        alt Key needs changing
                            TunnelBuilder -> KeyFactory : GetSharedKey()
                        end alt
                        TunnelBuilder -> rng : GenerateBlock()
                        note right : Create a new IV
                        TunnelBuilder -> encryptorCypher : SetKeyWithIV()

                        TunnelBuilder -> client : Read()
                        note over TunnelBuilder
                            Wait for incoming data
                        end note
                        TunnelBuilder -> encryptorCypher : PushData()
                        TunnelBuilder -> ITransfer : Transfer(encryptedData)

                    end loop

            @enduml
             */
            void EncodingWorker();

            /**
             * @brief ChangeKey
             * @param keyLifeSpan
             * @param bytesUsedOnKey
             * @param timeKeyGenerated
             * @return true if it is time to change the key
             */
            static bool ChangeKey(const remote::tunnels::KeyLifespan& keyLifeSpan,
                                  uint64_t bytesUsedOnKey,
                                  const std::chrono::high_resolution_clock::time_point& timeKeyGenerated);

            /// encryptor cypher
            std::unique_ptr<CryptoPP::AuthenticatedSymmetricCipher> encryptorCypher = nullptr;
            /// encryptor
            std::unique_ptr<CryptoPP::StreamTransformationFilter> encryptor = nullptr;
            /// decryptor cypher
            std::unique_ptr<CryptoPP::AuthenticatedSymmetricCipher> decryptorCypher = nullptr;
            /// decryptor
            std::unique_ptr<CryptoPP::AuthenticatedDecryptionFilter> decryptor = nullptr;

            /// endpoint to send/receive unencrypted data
            DeviceIO* client = nullptr;
            /// communication with peer
            std::shared_ptr<grpc::Channel> farSideEP;
            /// source of keys
            std::shared_ptr<grpc::Channel> myKeyFactoryChannel;
            /// when to change keys
            remote::tunnels::KeyLifespan currentKeyLifespan;
            /// peer for our keystore
            std::string currentKeyStoreTo;
            /// thread to read from device
            std::thread encodeThread;
            /// should the encryptor stop
            bool keepGoing = true;
            /// random number generator
            CryptoPP::RandomNumberGenerator* rng = nullptr;
            /// our server for our peer to connect to
            std::shared_ptr<grpc::Server> server;
            /// how to connect to our peer
            std::shared_ptr<grpc::ChannelCredentials> clientCreds;
        };

    }
}
