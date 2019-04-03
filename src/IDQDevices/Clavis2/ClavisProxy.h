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
#pragma once
#include <memory>                   // for shared_ptr
#include <stddef.h>                            // for size_t
#include <string>                              // for string
#include "CQPToolkit/Interfaces/IQKDDevice.h"  // for IQKDDevice
#include "QKDInterfaces/Site.pb.h"             // for Device
#include "Algorithms/Datatypes/URI.h"                          // for URI
#include "IDQDevices/idqdevices_export.h"

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{
    namespace session
    {
        class ClavisController;
    }
    namespace stats
    {
        class ReportServer;
    }
    class ISessionController;

    /**
     * @brief The ClavisProxy class
     * Connects to a clavis device my way of the cqp::IDQWrapper program and
     * cqp::remote::IIDQWrapper interface
     */
    class IDQDEVICES_EXPORT ClavisProxy : public virtual cqp::IQKDDevice
    {
    public:
        /// prefix for device uri
        static CONSTSTRING DriverName = "clavis";
        /// size of the secret key
        static const constexpr size_t InitialSecretKeyBytes = 32;
        /**
         * @brief ClavisProxy
         * @param initialConfig default settings for the device
         * @param creds
         * @param bytesPerKey
         */
        ClavisProxy(const remote::DeviceConfig& initialConfig, std::shared_ptr<grpc::ChannelCredentials> creds);

        // IQKDDevice interface
    public:
        /// @{
        /// @name IQKDDevice interface

        /// @copydoc cqp::IQKDDevice::GetDriverName
        std::string GetDriverName() const override;

        /// @copydoc cqp::IQKDDevice::GetAddress
        URI GetAddress() const override;

        /// @copydoc cqp::IQKDDevice::Initialise
        bool Initialise(const remote::SessionDetails& sessionDetails) override;

        /// @copydoc cqp::IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;

        /// @copydoc cqp::IQKDDevice::GetDeviceDetails
        cqp::remote::DeviceConfig GetDeviceDetails() override;

        /// @copydoc cqp::IQKDDevice::GetKeyPublisher
        KeyPublisher*GetKeyPublisher() override;
        /// @copydoc cqp::IQKDDevice::RegisterServices
        void RegisterServices(grpc::ServerBuilder& builder) override;

        void SetInitialKey(std::unique_ptr<PSK> initailKey) override;
        ///@}
    protected:
        /// controller which passes key from the wrapper
        std::shared_ptr<session::ClavisController> controller;
        remote::DeviceConfig config;

        std::shared_ptr<stats::ReportServer> reportServer;
        std::unique_ptr<PSK> authKey;
    };

} // namespace cqp


