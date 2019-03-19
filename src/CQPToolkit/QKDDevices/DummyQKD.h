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
#include <string>                              // for string
#include "CQPToolkit/Interfaces/IQKDDevice.h"  // for IQKDDevice
#include "QKDInterfaces/Site.pb.h"             // for Device, Side, Side::Type
#include "Algorithms/Datatypes/URI.h"               // for URI
#include "Algorithms/Random/RandomNumber.h"

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

        /**
         * @brief DummyQKD
         * Constructor
         * @param side Whether the device should be an alice or bob
         * @param creds credentials to use when talking to peer
         * @param bytesPerKey
         */
        DummyQKD(const remote::DeviceConfig& initialConfig, std::shared_ptr<grpc::ChannelCredentials> creds);

        /**
         * @brief DummyQKD
         * Construct using a device url
         * @param address The url of the device
         * @param creds credentials to use when talking to peer
         * @param bytesPerKey
         */
        DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds);

        /**
         * @brief ~DummyQKD
         * destructor
         */
        ~DummyQKD() override;


        // IQKDDevice interface
    public:
        /// @{
        /// @name IQKDDevice interface

        /// @copydoc IQKDDevice::SetInitialKey
        void SetInitialKey(std::unique_ptr<PSK> initialKey) override;

        /// @copydoc cqp::IQKDDevice::GetDriverName
        std::string GetDriverName() const override;

        /// @copydoc cqp::IQKDDevice::GetAddress
        URI GetAddress() const override;

        /// @copydoc cqp::IQKDDevice::Initialise
        bool Initialise(const remote::SessionDetails& sessionDetails) override;

        /// @copydoc cqp::IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;

        /// @copydoc cqp::IQKDDevice::GetDeviceDetails
        remote::DeviceConfig GetDeviceDetails() override;

        /// @copydoc cqp::IQKDDevice::GetKeyPublisher
        KeyPublisher* GetKeyPublisher() override;

        /// @copydoc cqp::IQKDDevice::GetStatsPublisher
        stats::IStatsPublisher* GetStatsPublisher() override;
        ///@}

    protected: // members
        /// randomness
        RandomNumber rng;

        /// handles postprocessing
        class ProcessingChain;

        /// handles postprocessing
        std::unique_ptr<ProcessingChain> processing;

        remote::DeviceConfig config;
    };

} // namespace cqp


