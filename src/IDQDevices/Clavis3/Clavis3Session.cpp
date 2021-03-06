/*!
* @file
* @brief Clavis3Session
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28/6/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#if defined(HAVE_IDQ4P)

#include "Clavis3Session.h"
#include "Clavis3/Clavis3SessionImpl.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "ClavisKeyFile.h"
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{

    Clavis3Session::Clavis3Session(const std::string& hostname,
                                   std::shared_ptr<grpc::ChannelCredentials> newCreds,
                                   std::shared_ptr<stats::ReportServer> theReportServer,
                                   bool disableControl, const std::string& keyFile) :
        session::SessionController {newCreds, {}, theReportServer},
            pImpl{std::make_unique<Clavis3Session::Impl>(hostname)},
            controlsEnabled(!disableControl)
    {
        if(reportServer)
        {
            pImpl->alignementStats.Add(reportServer.get());
            pImpl->errorStats.Add(reportServer.get());
            pImpl->clavis3Stats.Add(reportServer.get());
        }
        if(controlsEnabled)
        {
            pImpl->SubscribeToSignals();
            if(pImpl->GetState() != idq4p::domainModel::SystemState::PowerOff)
            {
                LOGINFO("Resetting system...");
                pImpl->Reboot();
            }
        }
        else
        {
            LOGWARN("Control signals disabled");
        }

        if(keyFile.empty())
        {
            keyPub = std::make_unique<KeyPublisher>();
        }
        else
        {
            keyPub = std::make_unique<ClavisKeyFile>(keyFile);
        }
    }

    Clavis3Session::~Clavis3Session()
    {
        if(reportServer)
        {
            pImpl->alignementStats.Remove(reportServer.get());
            pImpl->errorStats.Remove(reportServer.get());
            pImpl->clavis3Stats.Remove(reportServer.get());
        }
    }

    grpc::Status Clavis3Session::SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty* response)
    {
        LOGTRACE("");
        auto result = SessionController::SessionStarting(context, request, response);
        if(result.ok() && controlsEnabled)
        {
            pImpl->PowerOn();
        }
        pImpl->SetBobChannel(otherControllerChannel);

        keepReadingKeys = true;
        keyReader = std::thread(&Clavis3Session::PassOnKeys, this);

        return result;
    }

    grpc::Status Clavis3Session::SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response)
    {
        LOGTRACE("");
        auto result = SessionController::SessionEnding(context, request, response);

        if(controlsEnabled)
        {
            pImpl->Reboot();
        }
        keepReadingKeys = false;
        if(keyReader.joinable())
        {
            keyReader.join();
        }

        return result;
    }

    grpc::Status Clavis3Session::ReleaseKeys(grpc::ServerContext*, const remote::IdList* request, google::protobuf::Empty*)
    {
        LOGTRACE("");
        grpc::Status result;
        using namespace std;

        auto keysEmitted = make_unique<KeyList>();
        keysEmitted->reserve(request->id().size());

        unique_lock<mutex> lock(bufferedKeysMutex);
        std::map<UUID, PSK>::iterator keyValueIt;

        for(const auto& id : request->id())
        {
            bool success = bufferedKeysCv.wait_for(lock, chrono::seconds(10), [&]()
            {
                keyValueIt = bufferedKeys.find(id);
                return keyValueIt != bufferedKeys.end();
            });

            if(success)
            {
                keysEmitted->emplace_back(move(keyValueIt->second));
                bufferedKeys.erase(keyValueIt);
            }
            else
            {
                result = grpc::Status(grpc::StatusCode::NOT_FOUND, "Failed to find matching key within timeout");
                break; // for
            }
        }

        if(result.ok())
        {
            keyPub->Emit(&IKeyCallback::OnKeyGeneration, move(keysEmitted));
        }
        return result;
    }

    grpc::Status Clavis3Session::SendInitialKey(grpc::ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*)
    {
        pImpl->SendInitialKey();
        return grpc::Status();
    }

    grpc::Status Clavis3Session::StartSession(const remote::SessionDetailsFrom& sessionDetails)
    {
        LOGTRACE("");
        auto result = SessionController::StartSession(sessionDetails);
        if(result.ok() && controlsEnabled)
        {
            pImpl->PowerOn();
        }
        pImpl->SetBobChannel(otherControllerChannel);

        keepReadingKeys = true;
        keyReader = std::thread(&Clavis3Session::PassOnKeys, this);

        return result;
    }

    void Clavis3Session::EndSession()
    {
        LOGTRACE("");

        if(controlsEnabled && pImpl->GetState() != idq4p::domainModel::SystemState::NOT_DEFINED)
        {
            pImpl->Reboot();
        }

        keepReadingKeys = false;
        if(keyReader.joinable())
        {
            keyReader.join();
        }
        SessionController::EndSession();
    }

    remote::Side::Type Clavis3Session::GetSide() const
    {
        return pImpl->GetSide();
    }

    bool Clavis3Session::Initialise(const remote::SessionDetails& sessionDetails)
    {
        if(controlsEnabled)
        {
            // Dont know whether to call this - it looks like the cockpit software *doesn't* call it
            //pImpl->Zeroize();
            pImpl->Reboot();
        }

        return true;
    }

    void Clavis3Session::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        LOGTRACE("");
        if(controlsEnabled)
        {
            pImpl->SetInitialKey(move(initailKey));
        }
        else
        {
            LOGWARN("Controls disabled");
        }
    }

    void Clavis3Session::PassOnKeys()
    {
        LOGTRACE("");
        using namespace std;

        if(pImpl->GetSide() == remote::Side::Type::Side_Type_Alice)
        {
            using namespace grpc;
            auto bob = remote::ISync::NewStub(this->otherControllerChannel);
            ClavisKeyList keys;

            while(keepReadingKeys)
            {
                if(pImpl->ReadKeys(keys))
                {
                    auto keysEmitted = std::make_unique<KeyList>();
                    ClientContext ctx;
                    remote::IdList request;
                    google::protobuf::Empty response;

                    for(auto& key : keys)
                    {
                        request.add_id(key.first);
                        keysEmitted->emplace_back(key.second);
                    }

                    if(LogStatus(bob->ReleaseKeys(&ctx, request, &response)).ok())
                    {
                        keyPub->Emit(&IKeyCallback::OnKeyGeneration, move(keysEmitted));
                    }
                }
                keys.clear();
            }
        }
        else
        {
            using namespace grpc;
            ClavisKeyList keys;
            while(keepReadingKeys)
            {
                if(pImpl->ReadKeys(keys))
                {
                    /*lock scope*/
                    {
                        lock_guard<mutex> lock(bufferedKeysMutex);
                        for(auto& key : keys)
                        {
                            bufferedKeys.insert(key);
                        }
                    }/*lock scope*/

                    bufferedKeysCv.notify_all();
                }
                keys.clear();
            }
        }
    }

    KeyPublisher* Clavis3Session::GetKeyPublisher()
    {
        return keyPub.get();
    }

    bool Clavis3Session::SystemAvailable()
    {
        return pImpl->GetState() != idq4p::domainModel::SystemState::NOT_DEFINED;
    }

} // namespace cqp
#endif // HAVE_IDQ4P
