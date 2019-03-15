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
#include "Clavis3Device.h"
#include "Clavis3DeviceImpl.h"

namespace cqp
{
    Clavis3Device::Clavis3Device(const std::string& hostname) :
        pImpl{std::make_unique<Clavis3Device::Impl>(hostname)}
    {
    }

    std::string Clavis3Device::GetDriverName() const
    {
        return "Clavis3";
    }

    bool Clavis3Device::Initialise(config::DeviceConfig& parameters)
    {
        pImpl->SubscribeToStateChange();
        pImpl->Request_GetProtocolVersion();
        pImpl->Request_GetSoftwareVersion();
        pImpl->Request_GetBoardInformation();

        pImpl->Request_UpdateSoftware();
        pImpl->Request_Zeroize();
        pImpl->Request_SetInitialKey({0x1u,0x2u,0x3u,0x4u,0x5u,0x6u,0x7u,0x8u,0x9u,0xAu,0xBu,0xCu,0xDu,0xEu,0xFu});
        pImpl->Request_GetRandomNumber();
        pImpl->Request_PowerOn();

        return true;
    }

    IKeyPublisher* Clavis3Device::GetKeyPublisher()
    {
        return this;
    }

    grpc::Status Clavis3Device::StartServer(const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds)
    {
    }

    grpc::Status Clavis3Device::StartServerAndConnect(cqp::URI otherController, const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds)
    {
    }

    std::string Clavis3Device::GetConnectionAddress() const
    {
    }

    grpc::Status Clavis3Device::StartSession()
    {
    }

    void Clavis3Device::EndSession()
    {
    }

    void Clavis3Device::WaitForEndOfSession()
    {
    }

    void Clavis3Device::PassOnKeys()
    {
        KeyID id;
        auto keys = std::make_unique<KeyList>();
        keys->resize(1);
        if(pImpl->ReadKey(id, (*keys)[0]))
        {
            //Emit<&IKeyCallback::OnKeyGeneration>(move(keys));
            if(listener)
            {
                listener->OnKeyGeneration(move(keys));
            }
        }
    }

} // namespace cqp
