/*!
* @file
* @brief ClavisController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ClavisController.h"
#include "CQPToolkit/Drivers/DeviceFactory.h"
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Drivers/DeviceFactory.h"
#include "CQPToolkit/Drivers/ClavisProxy.h"
#include "CQPToolkit/Session/TwoWayServerConnector.h"
#include <future>

namespace cqp
{

    using google::protobuf::Empty;
    using grpc::Status;
    using grpc::StatusCode;
    using grpc::ClientContext;

    const int wrapperPort = 7000;
    CONSTSTRING wrapperPeerKey = "peer";

    ClavisController::ClavisController(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds) :
        SessionController(creds)
    {
        URI addressUri(address);
        channel = grpc::CreateChannel(addressUri.GetHostAndPort(), creds);
        wrapper = remote::IIDQWrapper::NewStub(channel);
        if(wrapper)
        {
            grpc::ClientContext ctx;
            google::protobuf::Empty request;

            LOGTRACE("Getting details from wrapper");
            Status detailsResult = LogStatus(
                                       wrapper->GetDetails(&ctx, request, &myWrapperDetails));

            if(detailsResult.ok())
            {
                std::string side ="Alice";
                if(myWrapperDetails.side() == remote::Side_Type::Side_Type_Bob)
                {
                    side = "Bob";
                }

                LOGINFO("Connected to " + side + " wrapper on: " + addressUri.GetHostAndPort() +
                        " with internal address: " +
                        myWrapperDetails.hostname() + ":" + std::to_string(myWrapperDetails.portnumber()));
            }
            else
            {
                myWrapperDetails.Clear();
                wrapper.reset(nullptr);
            }
        }
        else
        {
            LOGERROR("Invalid wrapper address");
        }
    }

    void ClavisController::ReadKey(std::unique_ptr<grpc::ClientReader<remote::SharedKey>> reader, std::unique_ptr<ClientContext>)
    {
        using namespace std;
        try
        {
            KeyList toEmit;
            mutex emitMutex;

            LOGTRACE("Waiting for key from wrapper");
            auto readerSubTask = std::async([&]()
            {
                remote::SharedKey incomming;
                while(keepGoing && reader && reader->Read(&incomming))
                {
                    LOGTRACE("Got key from wrapper");

                    {
                        unique_lock<mutex> lock(emitMutex);
                        toEmit.push_back(PSK(incomming.keyvalue().begin(), incomming.keyvalue().end()));
                    }

                    incomming.Clear();
                }
            });

            do
            {
                if(!toEmit.empty())
                {
                    LOGTRACE("Sending " + to_string(toEmit.size()) + " keys.");
                    unique_lock<mutex> lock(emitMutex);
                    if(listener)
                    {

                        listener->OnKeyGeneration(unique_ptr<KeyList>(new KeyList(toEmit)));
                    }

                }
                this_thread::sleep_for(chrono::milliseconds(10));
            }
            while(readerSubTask.wait_for(chrono::seconds(0)) != future_status::ready || !toEmit.empty());

            // control reaches here when both the task has finished and all keys have been sent

            if(reader)
            {
                reader->Finish();
            }
            LOGTRACE("Finished");
        }
        catch (const std::exception& e)
        {
            LOGERROR(e.what());
        }
    }

    grpc::Status ClavisController::StartSession(const remote::OpticalParameters& params)
    {
        LOGTRACE("Called");
        using namespace std;
        ClientContext controllerCtx;
        remote::SessionDetails request;
        Empty response;
        Status result;

        if(!wrapper)
        {
            result = Status(StatusCode::RESOURCE_EXHAUSTED, "No wrapper to connect to");
        }
        else
        {
            twoWayComm->WaitForClient();

            remote::IDQStartOptions options;
            auto secret = pubKeyServ->GetSharedSecret(keyToken);
            if(secret->SizeInBytes() >= ClavisProxy::InitialSecretKeyBytes)
            {
                options.mutable_initialsecret()->assign(secret->begin(), secret->begin() + ClavisProxy::InitialSecretKeyBytes);
            }
            else
            {
                LOGWARN("Initial secret too small");
                options.mutable_initialsecret()->assign(secret->begin(), secret->end());
            }

            (*request.mutable_peeraddress()) = myAddress;
            (*request.mutable_keytoken()) = keyToken;
            controllerCtx.AddMetadata(wrapperPeerKey, myWrapperDetails.hostname());

            if(myWrapperDetails.side() == remote::Side_Type::Side_Type_Alice)
            {
                // get the other side to launch bob first,
                // the IDQ alice program expects bob to be running

                LOGTRACE("Calling SessionStarting on other controller");

                auto channel = twoWayComm->WaitForClient();
                std::unique_ptr<remote::ISession::Stub> otherController;
                if(channel)
                {
                    otherController = remote::ISession::NewStub(channel);
                } // if channel

                if(otherController)
                {
                    result = otherController->SessionStarting(&controllerCtx, request, &response);

                    for(const auto& param : controllerCtx.GetServerTrailingMetadata())
                    {
                        if(param.first == wrapperPeerKey)
                        {
                            options.set_peerhostname(std::string(param.second.begin(), param.second.end()));
                            LOGDEBUG("Peer hostname:" + options.peerhostname());
                        } // if wrapper peer key
                    } // for params
                } // if otherController

            } // if side == alice

            // launch the clavis driver remotely
            std::unique_ptr<ClientContext> wrapperCtx(new ClientContext());
            options.set_lineattenuation(params.lineattenuation());
            options.set_peerwrapperport(wrapperPort);

            LOGTRACE("Calling wrapper StartQKDSequence");
            auto reader = wrapper->StartQKDSequence(wrapperCtx.get(), options);
            // this will block until the IDQ program is ready
            LOGTRACE("Waiting for metadata from wrapper");
            reader->WaitForInitialMetadata();

            keepGoing = true;
            LOGTRACE("Starting ReadKey Thread");
            readThread = std::thread(&ClavisController::ReadKey, this, std::move(reader), std::move(wrapperCtx));

            if(myWrapperDetails.side() != remote::Side_Type::Side_Type_Alice)
            {
                // Now that we've launched bob
                // tell the other side to start alice
                LOGTRACE("Calling SessionStarting on peer");

                auto channel = twoWayComm->WaitForClient();
                std::unique_ptr<remote::ISession::Stub> otherController;
                if(channel)
                {
                    otherController = remote::ISession::NewStub(channel);
                } // if channel

                if(otherController)
                {
                    result = otherController->SessionStarting(&controllerCtx, request, &response);
                } // if otherController
            } // if side != Alice
        } // if wrapper
        LOGTRACE("Finished");
        return result;
    } // StartSession

    IKeyPublisher* ClavisController::GetKeyPublisher()
    {
        return this;
    } // GetKeyPublisher

    grpc::Status ClavisController::SessionStarting(grpc::ServerContext* ctx, const remote::SessionDetails* request, google::protobuf::Empty*)
    {
        LOGTRACE("Called");
        Status result;
        pairedControllerUri = request->peeraddress();
        keyToken = request->keytoken();

        if(!wrapper)
        {
            result = Status(StatusCode::RESOURCE_EXHAUSTED, "No wrapper to connect to");
        }
        else
        {
            // launch the clavis driver remotely
            std::unique_ptr<ClientContext> wrapperCtx(new ClientContext());
            remote::IDQStartOptions options;

            auto secret = pubKeyServ->GetSharedSecret(keyToken);
            if(secret->SizeInBytes() >= ClavisProxy::InitialSecretKeyBytes)
            {
                options.mutable_initialsecret()->assign(secret->begin(), secret->begin() + ClavisProxy::InitialSecretKeyBytes);
            }
            else
            {
                LOGWARN("Initial secret too small");
                options.mutable_initialsecret()->assign(secret->begin(), secret->end());
            }
            options.set_lineattenuation(request->params().lineattenuation());
            for(const auto& param : ctx->client_metadata())
            {
                if(param.first == wrapperPeerKey)
                {
                    options.set_peerhostname(std::string(param.second.begin(), param.second.end()));
                    LOGDEBUG("Peer hostname:" + options.peerhostname());
                }
            }
            options.set_peerwrapperport(wrapperPort);

            LOGTRACE("Calling wrapper StartQKDSequence");
            auto reader = wrapper->StartQKDSequence(wrapperCtx.get(), options);
            LOGTRACE("Waiting for metadata from wrapper");
            // this will block until the IDQ program is ready
            reader->WaitForInitialMetadata();

            LOGTRACE("Starting ReadKey Thread");
            readThread = std::thread(&ClavisController::ReadKey, this, std::move(reader), std::move(wrapperCtx));

            ctx->AddTrailingMetadata(wrapperPeerKey, myWrapperDetails.hostname());
        } // if wrapper

        LOGTRACE("Finished");
        return result;
    }

    void ClavisController::EndSession()
    {
        keepGoing = false;
        if(readThread.joinable())
        {
            readThread.join();
        }
    } // SessionStarting

    grpc::Status ClavisController::SessionEnding(grpc::ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*)
    {
        Status result;
        keepGoing = false;
        if(readThread.joinable())
        {
            readThread.join();
        }
        return result;
    } // SessionEnding

    void ClavisController::RegisterServices(grpc::ServerBuilder&)
    {
        // Nothing to do
    }

    remote::Side::Type ClavisController::GetSide()
    {
        return myWrapperDetails.side();
    } // RegisterServices

    std::vector<stats::StatCollection*> ClavisController::GetStats() const
    {
        return
        {
            // todo
        };
    } // GetStats

} // namespace cqp
