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
#include "CQPToolkit/QKDDevices/DeviceUtils.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"

namespace cqp
{
    Clavis3Device::Clavis3Device(const std::string& hostname,
                                 std::shared_ptr<grpc::ChannelCredentials> newCreds,
                                 std::shared_ptr<stats::ReportServer> theReportServer) :
        sessionController(hostname, newCreds, theReportServer)
    {
        deviceConfig.set_side(sessionController.GetSide());
        deviceConfig.set_kind(GetDriverName());
        deviceConfig.set_bytesperkey(32);
        URI devUri = DeviceUtils::ConfigToUri(deviceConfig);
        devUri.SetHost(hostname);
        deviceConfig.set_id(DeviceUtils::GetDeviceIdentifier(devUri));
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
        return DeviceUtils::ConfigToUri(deviceConfig);
    }

    bool Clavis3Device::Initialise(const remote::SessionDetails& sessionDetails)
    {
        return sessionController.Initialise(sessionDetails);
    }

    ISessionController* Clavis3Device::GetSessionController()
    {
        return &sessionController;
    }

    KeyPublisher* Clavis3Device::GetKeyPublisher()
    {
        return sessionController.GetKeyPublisher();
    }

    remote::DeviceConfig Clavis3Device::GetDeviceDetails()
    {
        return deviceConfig;
    }

    void Clavis3Device::SetInitialKey(std::unique_ptr<PSK> initailKey)
    {
        sessionController.SetInitialKey(move(initailKey));
    }

    void Clavis3Device::RegisterServices(grpc::ServerBuilder&)
    {
        // nothing to do
    }

    bool Clavis3Device::SystemAvailable()
    {
        return sessionController.SystemAvailable();
    }
} // namespace cqp
#endif //HAVE_IDQ4P
