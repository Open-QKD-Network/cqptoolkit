/*!
* @file
* @brief ClavisProxy
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ClavisProxy.h"
#include <grpcpp/impl/codegen/status_code_enum.h>  // for StatusCode
#include <utility>                                 // for move
#include "CQPToolkit/QKDDevices/DeviceFactory.h"      // for DeviceFactory
#include "CQPToolkit/Session/ClavisController.h"   // for ClavisController
#include "CQPToolkit/Interfaces/IQKDDevice.h"                 // for IQKDDevice, IQKDDe...
#include "Algorithms/Logging/Logger.h"                           // for LOGTRACE

namespace cqp
{
    class ISessionController;

    ClavisProxy::ClavisProxy(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t):
        myAddress(address)
    {
        LOGTRACE("Creating controller");
        controller.reset(new session::ClavisController(address, creds));
    }

    void ClavisProxy::RegisterWithFactory()
    {
        // tell the factory how to create a DummyQKD by specifying a lambda function
        for(auto side : {remote::Side::Alice, remote::Side::Bob})
        {
            DeviceFactory::RegisterDriver(DriverName, side, [](const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)
            {
                return std::shared_ptr<IQKDDevice>(new ClavisProxy(address, creds, bytesPerKey));
            });
        }
    }

    std::string ClavisProxy::GetDriverName() const
    {
        return DriverName;
    }

    URI ClavisProxy::GetAddress() const
    {
        return myAddress;
    }

    bool ClavisProxy::Initialise(config::DeviceConfig& parameters)
    {
        return controller->Initialise(parameters);
    }

    ISessionController*ClavisProxy::GetSessionController()
    {
        return controller.get();
    }

    remote::Device ClavisProxy::GetDeviceDetails()
    {
        remote::Device result;
        URI addrUri(myAddress);

        result.set_id(DeviceFactory::GetDeviceIdentifier(addrUri));
        result.set_side(controller->GetSide());
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchName, *result.mutable_switchname());
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchPort, *result.mutable_switchport());
        result.set_kind(addrUri.GetScheme());

        return result;
    }

    IKeyPublisher* ClavisProxy::GetKeyPublisher()
    {
        return controller.get();
    }

    std::vector<stats::StatCollection*> ClavisProxy::GetStats()
    {
        return {};
    }
} // namespace cqp

