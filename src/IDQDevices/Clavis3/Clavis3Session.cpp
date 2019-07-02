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
#include "Clavis3Session.h"
#include "Clavis3/Clavis3SessionImpl.h"
#include "CQPToolkit/Statistics/ReportServer.h"

namespace cqp
{

    Clavis3Session::Clavis3Session(const std::string& hostname,
                                   std::shared_ptr<grpc::ChannelCredentials> newCreds,
                                   std::shared_ptr<stats::ReportServer> theReportServer) :
        session::SessionController {newCreds, {}, theReportServer},
            pImpl{std::make_unique<Clavis3Session::Impl>(hostname)},
            keyPub{std::make_unique<KeyPublisher>()}
    {
        if(reportServer)
        {
            pImpl->alignementStats.Add(reportServer.get());
            pImpl->errorStats.Add(reportServer.get());
        }
    }

    Clavis3Session::~Clavis3Session()
    {
        if(reportServer)
        {
            pImpl->alignementStats.Remove(reportServer.get());
            pImpl->errorStats.Remove(reportServer.get());
        }
    }

    grpc::Status Clavis3Session::SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty* response)
    {
        auto result = SessionController::SessionStarting(context, request, response);
        if(result.ok())
        {
            if(pImpl->GetSide() == remote::Side_Type::Side_Type_Alice)
            {
                pImpl->PowerOn();
            }
        }
        return result;
    }

    grpc::Status Clavis3Session::SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response)
    {
        auto result = SessionController::SessionEnding(context, request, response);

        //pImpl->PowerOff();
        pImpl->Reboot();

        return result;
    }

    grpc::Status Clavis3Session::StartSession(const remote::SessionDetailsFrom& sessionDetails)
    {
        auto result = SessionController::StartSession(sessionDetails);
        if(result.ok())
        {
            if(pImpl->GetSide() == remote::Side_Type::Side_Type_Alice)
            {
                pImpl->PowerOn();
            }
        }
        return result;
    }

    void Clavis3Session::EndSession()
    {
        //pImpl->PowerOff();
        pImpl->Reboot();
        SessionController::EndSession();
    }

    remote::Side::Type Clavis3Session::GetSide() const
    {
        return pImpl->GetSide();
    }

    bool Clavis3Session::Initialise(const remote::SessionDetails& sessionDetails)
    {
        //pImpl->Request_UpdateSoftware();
        //pImpl->Zeroize();

        //pImpl->GetRandomNumber();
        return true;
    }

    void Clavis3Session::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        pImpl->SetInitialKey(*initailKey);
    }

    void Clavis3Session::PassOnKeys()
    {
        auto keys = std::make_unique<KeyList>();
        keys->resize(1);
        if(pImpl->ReadKey((*keys)[0]))
        {
            keyPub->Emit(&IKeyCallback::OnKeyGeneration, move(keys));
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
