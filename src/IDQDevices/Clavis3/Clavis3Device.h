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
#pragma once
#if defined(HAVE_IDQ4P)

#include <string>
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "IDQDevices/idqdevices_export.h"
#include "QKDInterfaces/Site.pb.h"
#include "IDQDevices/Clavis3/Clavis3Session.h"

namespace cqp
{
    class IDQDEVICES_EXPORT Clavis3Device :
        public virtual IQKDDevice
    {
    public:
        Clavis3Device(const std::string& hostname,
                      std::shared_ptr<grpc::ChannelCredentials> creds,
                      std::shared_ptr<stats::ReportServer> theReportServer);
        ~Clavis3Device() override;

        ///@{
        /// @name IQKDDevice interface

        /// @copydoc IQKDDevice::GetDriverName
        std::string GetDriverName() const override;
        /// @copydoc IQKDDevice::GetAddress
        URI GetAddress() const override;
        /// @copydoc IQKDDevice::Initialise
        bool Initialise(const remote::SessionDetails& sessionDetails) override;
        /// @copydoc IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;
        /// @copydoc IQKDDevice::GetKeyPublisher
        KeyPublisher* GetKeyPublisher() override;
        /// @copydoc IQKDDevice::GetDeviceDetails
        remote::DeviceConfig GetDeviceDetails() override;
        /// @copydoc IQKDDevice::SetInitialKey
        void SetInitialKey(std::unique_ptr<PSK> initailKey) override;
        /// @copydoc IQKDDevice::RegisterServices
        void RegisterServices(grpc::ServerBuilder& builder) override;
        ///@}

        bool SystemAvailable();

    protected: // methods
    protected: // members
        remote::DeviceConfig deviceConfig;
        Clavis3Session sessionController;
    };

} // namespace cqp

#endif // HAVE_IDQ4P
