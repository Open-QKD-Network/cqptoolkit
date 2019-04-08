/*!
* @file
* @brief Clavis3Device
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 14/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#if defined(HAVE_IDQ4P)
#include "Clavis3Device.h"
#include "Clavis3DeviceImpl.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "CQPToolkit/QKDDevices/DeviceUtils.h"

namespace cqp
{
    Clavis3Device::Clavis3Device(const std::string& hostname, remote::Side::Type theSide,
                                 std::shared_ptr<grpc::ChannelCredentials> newCreds,
                                 std::shared_ptr<stats::ReportServer> theReportServer) :
        session::SessionController {newCreds, {}, theReportServer},
            pImpl{std::make_unique<Clavis3Device::Impl>(hostname, theSide)}
    {
        if(reportServer)
        {
            pImpl->alignementStats.Add(reportServer.get());
            pImpl->errorStats.Add(reportServer.get());
        }

        URI devUri;

        devUri.SetScheme("clavis3");
        devUri.SetHost(hostname);
        deviceConfig.set_side(pImpl->GetSide());
        deviceConfig.set_kind(devUri.GetScheme());
        deviceConfig.set_id(DeviceUtils::GetDeviceIdentifier(devUri));
        deviceConfig.set_bytesperkey(32);
    }

    Clavis3Device::~Clavis3Device()
    {
        if(reportServer)
        {
            pImpl->alignementStats.Remove(reportServer.get());
            pImpl->errorStats.Remove(reportServer.get());
        }
    }

    std::string Clavis3Device::GetDriverName() const
    {
        return "Clavis3";
    }

    URI Clavis3Device::GetAddress() const
    {
        // TODO
        return URI();
    }

    bool Clavis3Device::Initialise(const remote::SessionDetails& sessionDetails)
    {
        pImpl->SubscribeToSignals();
        //pImpl->Request_UpdateSoftware();
        pImpl->Zeroize();

        //pImpl->GetRandomNumber();

        return true;
    }

    ISessionController* Clavis3Device::GetSessionController()
    {
        return this;
    }

    KeyPublisher* Clavis3Device::GetKeyPublisher()
    {
        return this;
    }

    remote::DeviceConfig Clavis3Device::GetDeviceDetails()
    {
        return deviceConfig;
    }

    grpc::Status Clavis3Device::StartSession(const remote::SessionDetailsFrom& sessionDetails)
    {
        auto result = SessionController::StartSession(sessionDetails);
        if(result.ok())
        {
            pImpl->PowerOn();
        }
        return result;
    }

    void Clavis3Device::EndSession()
    {
        pImpl->PowerOff();
        SessionController::EndSession();
    }

    void Clavis3Device::PassOnKeys()
    {
        auto keys = std::make_unique<KeyList>();
        keys->resize(1);
        if(pImpl->ReadKey((*keys)[0]))
        {
            Emit(&IKeyCallback::OnKeyGeneration, move(keys));
        }
    }

    grpc::Status Clavis3Device::SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty* response)
    {
        auto result = SessionController::SessionStarting(context, request, response);
        if(result.ok())
        {
            pImpl->PowerOn();
        }
        return result;
    }

    grpc::Status Clavis3Device::SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response)
    {
        auto result = SessionController::SessionEnding(context, request, response);

        pImpl->PowerOff();

        return result;
    }

    void Clavis3Device::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        pImpl->SetInitialKey(*initailKey);
    }

    void Clavis3Device::RegisterServices(grpc::ServerBuilder& builder)
    {
        SessionController::RegisterServices(builder);
    }
} // namespace cqp
#endif //HAVE_IDQ4P
