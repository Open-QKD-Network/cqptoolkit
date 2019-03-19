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
#include "CQPToolkit/Session/ClavisController.h"   // for ClavisController
#include "CQPToolkit/Interfaces/IQKDDevice.h"                 // for IQKDDevice, IQKDDe...
#include "Algorithms/Logging/Logger.h"                           // for LOGTRACE
#include "CQPToolkit/Statistics/ReportServer.h"
#include "DeviceUtils.h"

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

    std::string ClavisProxy::GetDriverName() const
    {
        return DriverName;
    }

    URI ClavisProxy::GetAddress() const
    {
        return myAddress;
    }

    bool ClavisProxy::Initialise(const remote::SessionDetails& sessionDetails)
    {
        return controller->Initialise(sessionDetails);
    }

    ISessionController*ClavisProxy::GetSessionController()
    {
        return controller.get();
    }

    remote::DeviceConfig ClavisProxy::GetDeviceDetails()
    {
        remote::DeviceConfig result;
        URI addrUri(myAddress);

        result.set_id(DeviceUtils::GetDeviceIdentifier(addrUri));
        result.set_side(controller->GetSide());
        addrUri.GetFirstParameter(IQKDDevice::Parameters::switchName, *result.mutable_switchname());
        addrUri.GetFirstParameter(IQKDDevice::Parameters::switchPort, *result.mutable_switchport());
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

