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
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/QKDDevices/ClavisProxy.h"
#include "CQPToolkit/Session/TwoWayServerConnector.h"
#include "QKDInterfaces/IReporting.grpc.pb.h"
#include <future>
#include "CQPToolkit/Statistics/ReportServer.h"

namespace cqp
{
    namespace session
    {
        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::StatusCode;
        using grpc::ClientContext;

        const int wrapperPort = 7000;
        CONSTSTRING wrapperPeerKey = "peer";

        ClavisController::ClavisController(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds,
                                           std::shared_ptr<stats::ReportServer> theReportServer) :
            SessionController(creds, {}, {}, theReportServer),
            pubKeyServ{new PublicKeyService()}
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

        ClavisController::~ClavisController()
        {
            EndSession();
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

                        Emit(&IKeyCallback::OnKeyGeneration, std::make_unique<KeyList>(toEmit));
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

        void ClavisController::CollectStats()
        {
            using namespace std;
            auto statsSource = remote::IReporting::NewStub(channel);
            grpc::ClientContext ctx;
            remote::ReportingFilter filter;
            filter.set_listisexclude(true);

            if(statsSource)
            {
                auto reader = statsSource->GetStatistics(&ctx, filter);
                remote::SiteAgentReport report;
                while(reader && reader->Read(&report))
                {
                    // feed the data back to the site agent
                    reportServer->StatsReport(report);
                }
            }
        }

        grpc::Status ClavisController::StartSession(const remote::SessionDetails& sessionDetails)
        {
            LOGTRACE("Called");
            using namespace std;
            ClientContext controllerCtx;
            Empty response;
            Status result;

            if(!wrapper)
            {
                result = Status(StatusCode::RESOURCE_EXHAUSTED, "No wrapper to connect to");
                UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Connected, result.error_code());
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

                remote::SessionDetailsFrom request;
                (*request.mutable_details()) = sessionDetails;
                (*request.mutable_initiatoraddress()) = myAddress;
                //(*request.mutable_keytoken()) = keyToken;
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
                options.set_lineattenuation(sessionDetails.lineattenuation());
                options.set_peerwrapperport(wrapperPort);

                LOGTRACE("Calling wrapper StartQKDSequence");
                auto reader = wrapper->StartQKDSequence(wrapperCtx.get(), options);
                // this will block until the IDQ program is ready
                LOGTRACE("Waiting for metadata from wrapper");
                reader->WaitForInitialMetadata();

                keepGoing = true;
                LOGTRACE("Starting ReadKey Thread");
                readThread = std::thread(&ClavisController::ReadKey, this, std::move(reader), std::move(wrapperCtx));
                LOGTRACE("Starting CollectStats Thread");
                statsThread = std::thread(&ClavisController::CollectStats, this);

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

                UpdateStatus(remote::LinkStatus::State::LinkStatus_State_SessionStarted, result.error_code());
            } // if wrapper
            LOGTRACE("Finished");
            return result;
        } // StartSession

        grpc::Status ClavisController::SessionStarting(grpc::ServerContext* ctx, const remote::SessionDetailsFrom* request, google::protobuf::Empty*)
        {
            LOGTRACE("Called");
            Status result;
            pairedControllerUri = request->initiatoraddress();
            sessionDetails = request->details();
            //keyToken = request->keytoken();

            if(!wrapper)
            {
                result = Status(StatusCode::RESOURCE_EXHAUSTED, "No wrapper to connect to");
                UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Connected, result.error_code());
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
                options.set_lineattenuation(sessionDetails.lineattenuation());
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
                if(reader)
                {
                    LOGTRACE("Waiting for metadata from wrapper");
                    // this will block until the IDQ program is ready
                    reader->WaitForInitialMetadata();

                    LOGTRACE("Starting ReadKey Thread");
                    readThread = std::thread(&ClavisController::ReadKey, this, std::move(reader), std::move(wrapperCtx));

                    ctx->AddTrailingMetadata(wrapperPeerKey, myWrapperDetails.hostname());
                    UpdateStatus(remote::LinkStatus::State::LinkStatus_State_SessionStarted);
                } else {
                    result = Status(StatusCode::ABORTED, "Invalid reader");
                    UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening, result.error_code());
                }
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
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Connected);
        } // SessionStarting

        grpc::Status ClavisController::SessionEnding(grpc::ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*)
        {
            Status result;
            keepGoing = false;
            if(readThread.joinable())
            {
                readThread.join();
            }
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Connected);
            return result;
        } // SessionEnding

        remote::Side::Type ClavisController::GetSide()
        {
            return myWrapperDetails.side();
        }

        bool ClavisController::Initialise(const remote::SessionDetails& session)
        {
            sessionDetails = session;
            return true;
        } // RegisterServices

    } // namespace session
} // namespace cqp
