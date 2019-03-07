/*!
* @file
* @brief CQP Toolkit - Cheap and cheerful LED driver Mk1
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22 Sep 2016
* @author David Lowndes <david.lowndes@bristol.ac.uk>
*/

#pragma once
#include "CQPToolkit/Interfaces/IQKDDevice.h"        // for IQKDDevice
#include "Algorithms/Datatypes/URI.h"                                // for URI
#include "Algorithms/Random/RandomNumber.h"

namespace grpc {
    class ChannelCredentials;
}

namespace cqp
{
    class Serial;
    class Usb;
    class ISessionController;
    class LEDDriver;
    namespace session {
        class AliceSessionController;
    }

    /// A transmitter which uses both serial and usb deivces to send photons
    class CQPTOOLKIT_EXPORT LEDAliceMk1 :
            public virtual IQKDDevice
    {
    public:

        /**
         * @brief LEDAliceMk1
         * Constructor by a combination of detecting and opening from paths
         * @param controlName Serial port for configuring the device
         * @param usbSerialNumber The serial number for the usb device
         * @param creds credentials to use for connections
         */
        LEDAliceMk1(std::shared_ptr<grpc::ChannelCredentials> creds, const std::string& controlName = "", const std::string& usbSerialNumber = "");

        /**
         * @brief LEDAliceMk1
         * Constructor for specifing both the devices
         * @param controlPort Serial port for configuring the device
         * @param dataPort usb port for sending data
         * @param creds credentials to use for connections
         */
        LEDAliceMk1(std::shared_ptr<grpc::ChannelCredentials> creds, std::unique_ptr<Serial> controlPort, std::unique_ptr<Usb> dataPort);

        /// Destructor
        virtual ~LEDAliceMk1() override;

        /// Link to the device factory for automatic creation
        static void RegisterWithFactory();

        /// @name IQKDDevice interface
        ///@{

        /// @copydoc IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;
        /// @copydoc IQKDDevice::GetKeyPublisher
        IKeyPublisher*GetKeyPublisher() override;

        /// @copydoc IQKDDevice::GetDriverName
        virtual std::string GetDriverName() const override;

        /// @copydoc cqp::IQKDDevice::GetDeviceDetails
        remote::Device GetDeviceDetails() override;

        /// @copydoc cqp::IQKDDevice::GetAddress
        URI GetAddress() const override;
        /// @copydoc IQKDDevice::Initialise
        virtual bool Initialise(config::DeviceConfig& parameters) override;
        /// @copydoc IQKDDevice::GetStatsPublisher
        stats::IStatsPublisher* GetStatsPublisher() override;
        ///@}

    protected: // members

        /// What this driver is called
        static const std::string DriverName;

        /// randomness
        RandomNumber rng;
        /// For processing the incomming data
        class ProcessingChain;
        /// For processing the incomming data
        std::unique_ptr<ProcessingChain> processing;
        /// our session controller
        std::unique_ptr<session::AliceSessionController> sessionController;
        /// The driver which produces the photons
        std::shared_ptr<LEDDriver> driver;
    };
}
