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
#include "CQPToolkit/Statistics/ReportServer.h"

namespace cqp
{
    class ISessionController;

    ClavisProxy::ClavisProxy(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t):
        myAddress(address),
        reportServer(std::make_shared<stats::ReportServer>())
    {
        LOGTRACE("Creating controller");
        controller.reset(new session::ClavisController(address, creds, reportServer));
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

    bool ClavisProxy::Initialise(remote::DeviceConfig& parameters)
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

    KeyPublisher* ClavisProxy::GetKeyPublisher()
    {
        return controller.get();
    }

    stats::IStatsPublisher* ClavisProxy::GetStatsPublisher()
    {
        return reportServer.get();
    }

} // namespace cqp

