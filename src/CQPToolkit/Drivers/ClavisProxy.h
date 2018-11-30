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
                     // for CONSTSTRING
#include "CQPToolkit/Util/URI.h"                          // for URI
namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{
    class ClavisController;
    class ISessionController;

    /**
     * @brief The ClavisProxy class
     * Connects to a clavis device my way of the cqp::IDQWrapper program and
     * cqp::remote::IIDQWrapper interface
     */
    class CQPTOOLKIT_EXPORT ClavisProxy : public virtual cqp::IQKDDevice
    {
    public:
        /// prefix for device uri
        static CONSTSTRING DriverName = "clavis";
        /// size of the secret key
        static const constexpr size_t InitialSecretKeyBytes = 32;
        /**
         * @brief ClavisProxy
         * @param address The url of the IDQWrapper instance
         * @details
         *  URI fields:
         *      host = host accessible location of the IIDQWrapper interface, usually localhost
         *      port = host accessible port of the IIDQWrapper interface
         *          docker maps port 7000 to this port on the host
         *      parameters
         *          side = alice or bob
         * clavis://localhost:7001/?side=alice
         * @param creds
         * @param bytesPerKey
         */
        ClavisProxy(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey = 16);

        /// tell the factory how to create these devices
        static void RegisterWithFactory();

        // IQKDDevice interface
    public:
        /// @{
        /// @name IQKDDevice interface

        /// @copydoc cqp::IQKDDevice::GetDriverName
        std::string GetDriverName() const override;

        /// @copydoc cqp::IQKDDevice::GetAddress
        URI GetAddress() const override;

        /// @copydoc cqp::IQKDDevice::Initialise
        bool Initialise() override;

        /// @copydoc cqp::IQKDDevice::GetDescription
        std::string GetDescription() const override;

        /// @copydoc cqp::IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;

        /// @copydoc cqp::IQKDDevice::GetDeviceDetails
        remote::Device GetDeviceDetails() override;
        ///@}
    protected:
        /// controller which passes key from the wrapper
        std::shared_ptr<ClavisController> controller;
        /// The address of the wrapper
        std::string myAddress;
    };

} // namespace cqp


