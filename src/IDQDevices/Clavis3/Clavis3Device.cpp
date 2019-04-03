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

namespace cqp
{
    Clavis3Device::Clavis3Device(const std::string& hostname,
                                 std::shared_ptr<grpc::ChannelCredentials> newCreds,
                                 std::shared_ptr<stats::ReportServer> theReportServer) :
        session::SessionController {newCreds, {}, theReportServer},
            pImpl{std::make_unique<Clavis3Device::Impl>(hostname)}
    {
    }

    Clavis3Device::~Clavis3Device()
    {

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
        pImpl->SubscribeToStateChange();
        pImpl->Request_GetProtocolVersion();
        pImpl->Request_GetSoftwareVersion();
        pImpl->Request_GetBoardInformation();
        //pImpl->Request_UpdateSoftware();
        pImpl->Request_Zeroize();
        // This will move once the new session control changes are merged
        pImpl->Request_SetInitialKey({0x1u,0x2u,0x3u,0x4u,0x5u,0x6u,0x7u,0x8u,0x9u,0xAu,0xBu,0xCu,0xDu,0xEu,0xFu});
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
        remote::DeviceConfig result;
        //TODO
        return result;
    }

    grpc::Status Clavis3Device::StartSession(const remote::SessionDetailsFrom& sessionDetails)
    {
        auto result = SessionController::StartSession(sessionDetails);
        if(result.ok())
        {
            pImpl->Request_PowerOn();
        }
        return result;
    }

    void Clavis3Device::EndSession()
    {
        pImpl->Request_PowerOff();
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
            pImpl->Request_PowerOn();
        }
        return result;
    }

    grpc::Status Clavis3Device::SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response)
    {
        auto result = SessionController::SessionEnding(context, request, response);

        pImpl->Request_PowerOff();

        return result;
    }

    void Clavis3Device::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        pImpl->Request_SetInitialKey(*initailKey);
    }

} // namespace cqp
#endif //HAVE_IDQ4P
