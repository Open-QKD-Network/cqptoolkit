/*!
* @file
* @brief IDQWrapper
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "IDQWrapper.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Util/GrpcLogger.h"

#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>

#include "CQPToolkit/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "CQPToolkit/Util/URI.h"
#include <thread>

#include "CQPToolkit/Net/DNS.h"
#include "CQPAlgorithms/Datatypes/Keys.h"

using namespace cqp;
using grpc::Status;
using grpc::StatusCode;

Status IDQWrapper::GetDetails(grpc::ServerContext*, const google::protobuf::Empty*, remote::WrapperDetails* response)
{
    Status result;
    LOGTRACE("Called");
    std::string myHostname = net::GetHostname();
    LOGDEBUG("My hostname is: " + myHostname);
    net::IPAddress myIpAddress;
    if(net::ResolveAddress(myHostname, myIpAddress))
    {
        LOGTRACE("Using IP from hostname: " + myIpAddress.ToString());
        response->set_hostname(myIpAddress.ToString());
    }
    else
    {
        LOGERROR("Failed to resolve my own address");
        auto addresses = net::GetHostIPs();
        if(!addresses.empty())
        {
            LOGTRACE("Using first IP: " + addresses[0].ToString());
            response->set_hostname(addresses[0].ToString());
        }
        else
        {
            LOGTRACE("Using hostname: " + myHostname);
            response->set_hostname(myHostname);
        }
    }

    response->set_portnumber(portNumber);

    side = IDQSequenceLauncher::DeviceFound();
    if(side == IDQSequenceLauncher::DeviceType::Alice)
    {
        LOGINFO("My device is Alice");
        response->set_side(remote::Side_Type::Side_Type_Alice);
    }
    else if(side == IDQSequenceLauncher::DeviceType::Bob)
    {
        LOGINFO("My device is Bob");
        response->set_side(remote::Side_Type::Side_Type_Bob);
    }
    else
    {
        result = Status(StatusCode::RESOURCE_EXHAUSTED, "No device available");
    }
    return result;
}

grpc::Status IDQWrapper::StartQKDSequence(grpc::ServerContext* srvCtx,
        const remote::IDQStartOptions* request,
        ::grpc::ServerWriter<remote::SharedKey>* writer)
{
    LOGTRACE("Called");
    std::lock_guard<std::mutex> lock(oneAtATime);
    grpc::Status result;

    if(side != IDQSequenceLauncher::DeviceType::None)
    {

        try
        {
            LOGTRACE("Launching process...");
            IDQSequenceLauncher launcher({request->initialsecret().begin(), request->initialsecret().end()}, request->peerhostname(), request->lineattenuation());
            LOGTRACE("Starting Clavis driver");
            // start communicating with the device
            device.reset(new Clavis(request->peerhostname(), side == IDQSequenceLauncher::DeviceType::Alice));

            device->SetRequestRetryLimit(3);
            LOGTRACE("Sending initial metadata");
            // tell the caller it's ok to continue
            writer->SendInitialMetadata();


            LOGTRACE("Testing device type");
            if(side == IDQSequenceLauncher::DeviceType::Alice)
            {
                LOGTRACE("Device is Alice");
                bool keepGoing = true;
                Clavis::ClavisKeyID id;
                PSK key;
                // connect to our peer
                grpc::ClientContext ctx;
                google::protobuf::Empty response;
                LOGDEBUG("Creating channel to Bob at " + request->peerhostname() + ":" + std::to_string(request->peerwrapperport()));
                auto peerChannel = grpc::CreateChannel(request->peerhostname() + ":" + std::to_string(request->peerwrapperport()), LoadChannelCredentials(creds));
                auto bobWrapper = remote::IIDQWrapper::NewStub(peerChannel);

                LOGTRACE("Creating UseKey writer");
                // get a writer to send key ids to
                auto bobWriter = bobWrapper->UseKeyID(&ctx, &response);
                // wait for bob's call to StartQKDSequence
                bobWriter->WaitForInitialMetadata();

                LOGTRACE("Waiting for key");
                // we'll request new keys and send the id to bob
                while(keepGoing && bobWriter && !srvCtx->IsCancelled())
                {
                    if(device->GetNewKey(key, id))
                    {
                        LOGDEBUG("Got key from device: " + key.ToString());
                        remote::SharedKey message;
                        remote::KeyIdValue bobKeyIdMessage;
                        message.set_keyid(id);
                        message.mutable_keyvalue()->assign(key.begin(), key.end());
                        bobKeyIdMessage.set_keyid(id);

                        LOGTRACE("Sending id to bob");
                        // send out the key id for bob to retrieve
                        keepGoing = bobWriter->Write(bobKeyIdMessage);
                        // send the key to our caller
                        LOGTRACE("Sending key to caller");
                        writer->Write(message);
                        key.clear();
                    }
                    else
                    {
                        LOGDEBUG("Failed to get key, retrying");
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                }

                if(srvCtx->IsCancelled())
                {
                    ctx.TryCancel();
                }
            } // device == alice
            else
            {
                keepReadingKey = true;
                LOGTRACE("Device is Bob");
                deviceReadyCv.notify_one();

                while(keepReadingKey && srvCtx && !srvCtx->IsCancelled())
                {
                    // we'll wait for IDs from alice
                    std::unique_lock<std::mutex> lock(waitingKeyIdsMutex);

                    LOGTRACE("Waiting for key id");
                    bool dataReady = waitingKeyIdsCv.wait_for(lock, std::chrono::seconds(10), [&]()
                    {
                        return !waitingKeyIds.empty();
                    });

                    // wait for a key id to be sent to us from alice
                    if(dataReady)
                    {
                        LOGTRACE("Received key id");
                        Clavis::ClavisKeyID id;
                        PSK key;

                        id = waitingKeyIds.front();
                        waitingKeyIds.pop();
                        // no longer need the lock on the list
                        lock.unlock();

                        LOGTRACE("Getting existing key");
                        if(device->GetExistingKey(key, id))
                        {
                            LOGDEBUG("Got key from device");
                            remote::SharedKey message;
                            message.set_keyid(id);
                            message.mutable_keyvalue()->assign(key.begin(), key.end());

                            LOGTRACE("Sending key to caller");
                            writer->Write(message);
                        }
                        else
                        {
                            LOGERROR("Failed to get existing key");
                        } // if else
                    } // if data ready
                } // whiel keep reading
            } // else device is bob
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
    } // if device valid
    else
    {
        result = grpc::Status(grpc::StatusCode::UNAVAILABLE, "No Clavis device found.");
    }

    LOGTRACE("Finished");
    return result;
} // StartQKDSequence

grpc::Status IDQWrapper::UseKeyID(grpc::ServerContext* ctx,
                                  ::grpc::ServerReader<remote::KeyIdValue>* reader,
                                  google::protobuf::Empty*)
{
    grpc::Status result;
    LOGTRACE("Called");
    std::unique_lock<std::mutex> lock(deviceReadyMutex);

    try
    {
        // wait for our StartQKDSequence to be called
        bool deviceReady = deviceReadyCv.wait_for(lock, std::chrono::seconds(30), [&]()
        {
            return device != nullptr;
        });

        if(deviceReady)
        {
            // release the caller
            reader->SendInitialMetadata();

            remote::KeyIdValue keyId {};
            while(reader->Read(&keyId))
            {
                LOGDEBUG("Got key from Alice");
                /*lock scope*/
                {
                    std::lock_guard<std::mutex> lock(waitingKeyIdsMutex);
                    waitingKeyIds.push(keyId.keyid());
                } /*lock scope*/
                waitingKeyIdsCv.notify_one();
            }
        }
        else
        {
            result = Status(StatusCode::UNAVAILABLE, "No device configured within timeout.");
            ctx->TryCancel();
        }
    }
    catch(const std::exception& e)
    {
        LOGERROR(e.what());
    }

    keepReadingKey = false;
    LOGTRACE("Finished");
    return result;
} // UseKeyID

struct Names
{
    static CONSTSTRING port = "port";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
};

IDQWrapper::IDQWrapper()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    GrpcAllowMACOnlyCiphers();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&IDQWrapper::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::port, "p", "Port number to listen on, Default = " + std::to_string(portNumber))
    .Bind();

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&IDQWrapper::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&IDQWrapper::HandleVerbose, this, _1));

}

void IDQWrapper::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Bridges communication between CQP Site Agents and the Clavis devices.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

int IDQWrapper::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(Names::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(Names::tls))
        {
            creds.set_usetls(true);
        }

        definedArguments.GetProp(Names::port, portNumber);

        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:" + std::to_string(portNumber), LoadServerCredentials(creds), &portNumber);

        LOGTRACE("Registering services");
        // Register services
        builder.RegisterService(this);
        // ^^^ Add new services here ^^^ //

        server = builder.BuildAndStart();

        if(server)
        {
            LOGDEBUG("Server started");
            LOGINFO("My address is: " + net::GetHostname() + ":" + std::to_string(portNumber));

            server->Wait();
        }
        else
        {
            LOGERROR("Failed to start server");
        }
    }

    return exitCode;
}

CQP_MAIN(IDQWrapper)
