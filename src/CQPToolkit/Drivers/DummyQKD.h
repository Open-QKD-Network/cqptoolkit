/*!
* @file
* @brief DummyQKD
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <memory>                   // for shared_ptr
#include <stddef.h>                            // for size_t
#include <string>                              // for string
#include "CQPToolkit/Interfaces/IQKDDevice.h"  // for IQKDDevice
#include "QKDInterfaces/Site.pb.h"             // for Device, Side, Side::Type
#include "CQPToolkit/Util/URI.h"               // for URI

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{
    class ISessionController;
    class SessionController;

    /**
     * @brief The DummyQKD class
     * QKD Device for testing
     */
    class CQPTOOLKIT_EXPORT DummyQKD : public virtual cqp::IQKDDevice
    {
    public:
        /// Driver name
        static constexpr const char* DriverName = "dummyqkd";

        /// tell the factory how to create these devices
        static void RegisterWithFactory();

        /**
         * @brief DummyQKD
         * Constructor
         * @param side Whether the device should be an alice or bob
         * @param creds credentials to use when talking to peer
         * @param bytesPerKey
         */
        DummyQKD(const remote::Side::Type& side, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey = 16);

        /**
         * @brief DummyQKD
         * Construct using a device url
         * @param address The url of the device
         * @param creds credentials to use when talking to peer
         * @param bytesPerKey
         */
        DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey = 16);

        /**
         * @brief ~DummyQKD
         * destructor
         */
        ~DummyQKD() override {}


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
        /// The controller managing this device
        SessionController* controller = nullptr;
        /// The address to use to contact this
        std::string myAddress;
    };

} // namespace cqp


