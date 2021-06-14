/*!
* @file
* @brief SessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SessionController.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Random/RandomNumber.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "Algorithms/Datatypes/Units.h"

namespace cqp
{
    namespace session
    {
        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::StatusCode;
        using grpc::ClientContext;

        SessionController::SessionController(std::shared_ptr<grpc::ChannelCredentials> creds,
                                             const RemoteCommsList& remotes,
                                             std::shared_ptr<stats::ReportServer> theReportServer):
            creds{creds},
            remoteComms{remotes},
            reportServer(theReportServer)
        {

        } // SessionController

        void SessionController::RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(this);
        }

        grpc::Status SessionController::StartSession(const remote::SessionDetailsFrom& sessionDetails)
        {
            LOGTRACE("");
            Status result;
            // the local system is starting the session
            // make sure the other side has connected to us

            auto otherController = remote::ISession::NewStub(otherControllerChannel);

            if(otherControllerChannel)
            {
                // Tell our listeners the new settings for the session
                EmitNewSession(sessionDetails);

                ClientContext ctx;
                Empty response;

                // send the command to the other side
                result = LogStatus(otherController->SessionStarting(&ctx, sessionDetails, &response));

                if(result.ok())
                {
                    for(auto& dependant : remoteComms)
                    {
                        dependant->Connect(otherControllerChannel);
                    }
                    if(reportServer)
                    {
                        reportServer->AddAdditionalProperties(PropertyNames::sessionActive, "true");
                    }

                    UpdateStatus(remote::LinkStatus::State::LinkStatus_State_SessionStarted);
                }
                else
                {
                    UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening, result.error_code());
                }
            } // if(otherController)
            else
            {
                result = Status(StatusCode::FAILED_PRECONDITION, "invalid remote session controller");
                UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening, result.error_code());
            } // else

            LOGTRACE("Ending");
            return result;
        } // StartSession

        void SessionController::EndSession()
        {
            LOGTRACE("");
            Empty request;
            Empty response;
            ClientContext ctx;

            if(reportServer)
            {
                reportServer->AddAdditionalProperties(PropertyNames::sessionActive, "false");
            }

            std::unique_ptr<remote::ISession::Stub> otherController;
            if(otherControllerChannel)
            {
                otherController = remote::ISession::NewStub(otherControllerChannel);
            }

            if(otherController)
            {
                // send the command to the other side
                LogStatus(
                    otherController->SessionEnding(&ctx, request, &response));
            } // if otherController

            if(reportServer)
            {
                // notify the state change
                remote::SiteAgentReport report;
                reportServer->StatsReport(report);
            }
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening);

            LOGTRACE("Ending");
        } // EndSession

        SessionController::~SessionController()
        {
            SessionController::EndSession();
            Disconnect();
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Inactive);

        } // ~SessionController

        Status SessionController::SessionStarting(grpc::ServerContext*, const remote::SessionDetailsFrom* sessionDetails, Empty*)
        {
            LOGTRACE("");
            using namespace std::chrono;
            // The session has been started remotly
            Status result;

            // if we havn't got a connection to the caller yet, create one
            if(!otherControllerChannel)
            {
                LOGDEBUG("Connecting to peer at " + sessionDetails->initiatoraddress());
                grpc::ChannelArguments args;
                args.SetMaxReceiveMessageSize(8_MiB);
                otherControllerChannel = grpc::CreateCustomChannel(sessionDetails->initiatoraddress(), creds, args);

                if(!otherControllerChannel->WaitForConnected(system_clock::now() + seconds(10)))
                {
                    result = Status(StatusCode::DEADLINE_EXCEEDED, "Failed to connect to peer");
                    UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening, result.error_code());
                }
            }

            if(result.ok())
            {
                if(reportServer)
                {
                    reportServer->AddAdditionalProperties(PropertyNames::sessionActive, "true");
                    reportServer->AddAdditionalProperties(PropertyNames::to, sessionDetails->initiatoraddress());
                }

                // Tell our listeners the new settings for the session
                EmitNewSession(*sessionDetails);

                // we connect here because this is the first we know that the other side is talking to us.
                for(auto& dependant : remoteComms)
                {
                    dependant->Connect(otherControllerChannel);
                }

                UpdateStatus(remote::LinkStatus::State::LinkStatus_State_SessionStarted);
            }

            LOGTRACE("Ending");
            return result;
        } // SessionStarting

        Status SessionController::SessionEnding(grpc::ServerContext*, const Empty*, Empty*)
        {
            LOGTRACE("");
            if(reportServer)
            {
                reportServer->AddAdditionalProperties(PropertyNames::sessionActive, "false");
            }

            if(reportServer)
            {
                // notify the state change
                remote::SiteAgentReport report;
                reportServer->StatsReport(report);
            }
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Listening);

            for(auto& dependant : remoteComms)
            {
                dependant->Disconnect();
            }
            // Tell our listeners that the session has ended
            EmitSessionHasEnded();

            LOGTRACE("Ending");
            return Status();
        }

        void SessionController::UpdateStatus(remote::LinkStatus::State newState, int errorCode)
        {

            {
                /*lock scope*/
                std::unique_lock<std::mutex> lock(threadControlMutex);
                sessionState.set_state(newState);
                sessionState.set_errorcode(errorCode);
            }/*lock scope*/
            linkStatusCv.notify_all();
        } // SessionEnding

        void SessionController::EmitNewSession(const remote::SessionDetailsFrom& sessionDetails)
        {
            for(auto listener : listeners)
            {
                try
                {
                    listener->NewSessionDetails(sessionDetails);
                }
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                }
            }
        } // EmitNewSession

        void SessionController::EmitSessionHasEnded()
        {
            for(auto listener : listeners)
            {
                try
                {
                    listener->SessionHasEnded();
                }
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                }
            }
        } // EmitSessionHasEnded

        grpc::Status SessionController::Connect(const URI& otherController)
        {
            using namespace std;
            using namespace std::chrono;
            grpc::Status result;

            // make sure we're disconnected
            Disconnect();
            pairedControllerUri = otherController;

            grpc::ChannelArguments args;
            args.SetMaxReceiveMessageSize(8_MiB);
            otherControllerChannel = grpc::CreateCustomChannel(otherController, creds, args);

            if(reportServer)
            {
                reportServer->AddAdditionalProperties(PropertyNames::to, otherController);
            }

            for(auto& dependant : remoteComms)
            {
                dependant->Connect(otherControllerChannel);
            }
            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_Connected);

            return result;
        } // Connect

        void SessionController::Disconnect()
        {

            for(auto& dependant : remoteComms)
            {
                if(dependant)
                {
                    dependant->Disconnect();
                }
            }

            otherControllerChannel.reset();
            pairedControllerUri.clear();
        }

        grpc::Status SessionController::GetLinkStatus(grpc::ServerContext* context, ::grpc::ServerWriter<remote::LinkStatus>* writer)
        {
            grpc::Status result;
            using namespace std;
            remote::LinkStatus lastState;
            bool keepGoing = true;

            {
                /*lockscope*/
                lock_guard<mutex> lock(threadControlMutex);
                lastState = sessionState;
            }/*lockscope*/
            // send the current state
            keepGoing = writer->Write(lastState);

            while(keepGoing && !context->IsCancelled())
            {
                {
                    /*lockscope*/

                    unique_lock<mutex> lock(threadControlMutex);
                    linkStatusCv.wait(lock, [&]()
                    {
                        return context->IsCancelled() ||
                               sessionState.state() != lastState.state() ||
                               sessionState.errorcode() != lastState.errorcode();
                    });
                    lastState = sessionState;
                }/*lockscope*/

                keepGoing = writer->Write(lastState);
            } // while

            return result;
        } // Connect

    } // namespace session
} // namespace cqp
