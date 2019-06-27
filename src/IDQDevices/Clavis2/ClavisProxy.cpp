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
#include "IDQDevices/Clavis2/ClavisController.h"   // for ClavisController
#include "CQPToolkit/Interfaces/IQKDDevice.h"                 // for IQKDDevice, IQKDDe...
#include "Algorithms/Logging/Logger.h"                           // for LOGTRACE
#include "CQPToolkit/Statistics/ReportServer.h"
#include "CQPToolkit/QKDDevices/DeviceUtils.h"

namespace cqp
{
    class ISessionController;

    ClavisProxy::ClavisProxy(const remote::DeviceConfig& initialConfig,
                             std::shared_ptr<grpc::ChannelCredentials> creds):
        config(initialConfig),
        reportServer(std::make_shared<stats::ReportServer>())
    {
        LOGTRACE("Creating controller");
        config.set_kind(DriverName);
        if(config.id().empty())
        {
            config.set_id(DeviceUtils::GetDeviceIdentifier(GetAddress()));
        }
        controller.reset(new session::ClavisController(creds, reportServer));
        config.set_side(controller->GetSide());
    }

    std::string ClavisProxy::GetDriverName() const
    {
        return DriverName;
    }

    URI ClavisProxy::GetAddress() const
    {
        return DeviceUtils::ConfigToUri(config);
    }

    bool ClavisProxy::Initialise(const remote::SessionDetails& sessionDetails)
    {
        return controller->Initialise(move(authKey));
    }

    ISessionController*ClavisProxy::GetSessionController()
    {
        return controller.get();
    }

    remote::DeviceConfig ClavisProxy::GetDeviceDetails()
    {
        return config;
    }

    KeyPublisher* ClavisProxy::GetKeyPublisher()
    {
        return controller.get();
    }

    /// @copydoc cqp::IQKDDevice::RegisterServices
    void ClavisProxy::RegisterServices(grpc::ServerBuilder& builder)
    {
        builder.RegisterService(reportServer.get());
    }

    void ClavisProxy::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        authKey = move(initailKey);
    }

} // namespace cqp


