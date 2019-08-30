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
#include "IDQDevices/Clavis2/ClavisProxy.h"
#include "IDQDevices/Clavis2/Clavis.h"
#include "QKDInterfaces/IReporting.grpc.pb.h"
#include <future>
#include "CQPToolkit/Statistics/ReportServer.h"
#include "IDQDevices/Clavis2/IDQSequenceLauncher.h"

namespace cqp
{
    namespace session
    {
        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::StatusCode;
        using grpc::ClientContext;

        ClavisController::ClavisController( std::shared_ptr<grpc::ChannelCredentials> creds,
                                            std::shared_ptr<stats::ReportServer> theReportServer) :
            SessionController(creds,
        {}, theReportServer)
        {
            LOGTRACE("Getting details from launcher");
            auto devSide = IDQSequenceLauncher::DeviceFound();

            if(devSide != IDQSequenceLauncher::DeviceType::None)
            {
                side = remote::Side_Type::Side_Type_Alice;
                std::string sideStr ="Alice";
                if(devSide == IDQSequenceLauncher::DeviceType::Bob)
                {
                    side = remote::Side_Type::Side_Type_Bob;
                    sideStr = "Bob";
                }

                LOGINFO("Connected to " + sideStr);
            }
            else
            {
                LOGERROR("No device found.");
            }
        }

        ClavisController::~ClavisController()
        {
            EndSession();
        }

        void ClavisController::ReadKey()
        {
            using namespace std;
            try
            {
                KeyList toEmit;
                remote::KeyIdValueList idList;
                mutex emitMutex;

                LOGTRACE("Waiting for key from wrapper");

                PSK keyValue;
                KeyID keyId;
                // connect to the other controller
                auto peer = remote::IIDQWrapper::NewStub(otherControllerChannel);

                grpc::ClientContext ctx;
                google::protobuf::Empty response;
                auto keyIdWriter = peer->UseKeyID(&ctx, &response);

                auto readerSubTask = std::async(launch::async, [&]()
                {
                    while(keepGoing)
                    {
                        if(launcher->WaitForKey())
                        {
                            while(device->GetNewKey(keyValue, keyId))
                            {
                                LOGTRACE("Got key from wrapper");

                                {
                                    unique_lock<mutex> lock(emitMutex);
                                    toEmit.push_back(keyValue);
                                    idList.add_keyid(keyId);
                                }

                                keyValue.clear();
                                keyId = 0;
                            }
                        }
                    } // while keep going
                });

                do
                {
                    if(!toEmit.empty())
                    {
                        unique_lock<mutex> lock(emitMutex);
                        LOGTRACE("Sending " + to_string(toEmit.size()) + " keys.");
                        if(keyIdWriter->Write(idList))
                        {
                            Emit(&IKeyCallback::OnKeyGeneration, std::make_unique<KeyList>(toEmit));
                            toEmit.clear();
                        }
                        else
                        {
                            keepGoing = false;
                            LOGINFO("Key id link closed");
                        }

                    }
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                while(readerSubTask.wait_for(chrono::seconds(0)) != future_status::ready || !toEmit.empty());

                // control reaches here when both the task has finished and all keys have been sent

                LOGTRACE("Finished");
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }

        void ClavisController::RegisterServices(grpc::ServerBuilder &builder)
        {
            SessionController::RegisterServices(builder);
            builder.RegisterService(static_cast<remote::IIDQWrapper::Service*>(this));
        }

        grpc::Status ClavisController::StartSession(const remote::SessionDetailsFrom& sessionDetails)
        {
            LOGTRACE("Called");

            // start the bob driver before telling alice to start
            if(side == remote::Side_Type::Side_Type_Bob)
            {
                LOGTRACE("Launching bob process...");
                launcher = std::make_unique<IDQSequenceLauncher>(*authKey,
                           pairedControllerUri,
                           sessionDetails.details().lineattenuation());
            }

            auto result = SessionController::StartSession(sessionDetails);

            using namespace std;

            if(side == remote::Side_Type::Side_Type_Alice)
            {
                LOGTRACE("Launching alice process...");
                launcher = std::make_unique<IDQSequenceLauncher>(*authKey,
                           pairedControllerUri,
                           sessionDetails.details().lineattenuation());
            } // device == alice

            LOGTRACE("Starting Clavis driver");
            // start communicating with the device
            device.reset(new Clavis("localhost", side == remote::Side_Type::Side_Type_Alice));
            // publish stats from the device
            launcher->stats.Add(reportServer.get());

            device->SetRequestRetryLimit(3);

            if(side == remote::Side_Type::Side_Type_Alice)
            {
                keepGoing = true;
                LOGTRACE("Starting ReadKey Thread");
                readThread = std::thread(&ClavisController::ReadKey, this);
            } // device == alice

            UpdateStatus(remote::LinkStatus::State::LinkStatus_State_SessionStarted, result.error_code());

            LOGTRACE("Finished");
            return result;
        } // StartSession

        grpc::Status ClavisController::SessionStarting(grpc::ServerContext* ctx, const remote::SessionDetailsFrom* request, google::protobuf::Empty* response)
        {
            LOGTRACE("Called");
            auto result = SessionController::SessionStarting(ctx, request, response);

            if(result.ok())
            {
                //keyToken = request->keytoken();

                LOGTRACE("Launching bob process...");
                launcher = std::make_unique<IDQSequenceLauncher>(*authKey, request->initiatoraddress(),
                           request->details().lineattenuation());
                LOGTRACE("Starting Clavis driver");
                // start communicating with the device
                device.reset(new Clavis("localhost", side == remote::Side_Type::Side_Type_Alice));
                // publish stats from the device
                launcher->stats.Add(reportServer.get());

                device->SetRequestRetryLimit(3);

                if(side == remote::Side_Type::Side_Type_Alice)
                {
                    keepGoing = true;
                    LOGTRACE("Starting ReadKey Thread");
                    readThread = std::thread(&ClavisController::ReadKey, this);
                } // device == alice

            }
            LOGTRACE("Finished");
            return result;
        }

        void ClavisController::EndSession()
        {
            keepGoing = false;
            launcher.reset();
            device.reset();
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
            return side;
        }

        bool ClavisController::Initialise( std::unique_ptr<PSK> initialKey)
        {
            authKey = move(initialKey);
            return true;
        } // RegisterServices

        Status ClavisController::UseKeyID(grpc::ServerContext*,
                                          ::grpc::ServerReader<remote::KeyIdValueList>* reader, google::protobuf::Empty*)
        {
            Status result;

            if(side == remote::Side_Type::Side_Type_Bob)
            {
                if(device)
                {
                    remote::KeyIdValueList keys;
                    while(reader->Read(&keys))
                    {
                        KeyList toEmit;
                        toEmit.reserve(keys.keyid().size());

                        for (const auto& keyId : keys.keyid())
                        {
                            toEmit.emplace_back(PSK());
                            device->GetExistingKey(toEmit.back(), keyId);
                        }

                        Emit(&IKeyCallback::OnKeyGeneration, std::make_unique<KeyList>(toEmit));
                        toEmit.clear();
                    }
                }
                else
                {
                    result = Status(StatusCode::UNAVAILABLE, "No device configured");
                }
            }
            else
            {
                result = Status(StatusCode::DO_NOT_USE, "Should only be called on Bob");
            }
            return result;
        }
    } // namespace session
} // namespace cqp
