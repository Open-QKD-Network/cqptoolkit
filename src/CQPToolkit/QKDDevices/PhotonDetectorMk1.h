/*!
* @file
* @brief CQP Toolkit - Photon Detection
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/IQKDDevice.h"             // for IQKDDevice
#include "Algorithms/Datatypes/URI.h"                                     // for URI

namespace grpc {
    class ChannelCredentials;
}

namespace cqp
{
    class UsbTagger;
    class Usb;
    class Serial;

    namespace session {
        class SessionController;
    }

    /// Processes detections from the low power detector
    /// @todo come up with a name for this device
    class CQPTOOLKIT_EXPORT PhotonDetectorMk1 : public virtual IQKDDevice
    {
    public:

        /// tell the factory how to create these devices
        static void RegisterWithFactory();

        /**
         * @brief PhotonDetectorMk1
         * Constructor which opens a device
         */
        explicit PhotonDetectorMk1(std::shared_ptr<grpc::ChannelCredentials> creds, const std::string& controlName = "", const std::string& usbSerialNumber = "");

        /**
         * @brief PhotonDetectorMk1
         * Open a device which uses both usb and serial
         * @param usbDev usb data transfer device
         * @param serialDev serial control device
         */
        PhotonDetectorMk1(std::shared_ptr<grpc::ChannelCredentials> creds, std::unique_ptr<Serial> serialDev, std::unique_ptr<cqp::Usb> usbDev);

        /**
         * Destructor
         */
        ~PhotonDetectorMk1() override = default;

        /// @name IQKDDevice interface
        /// @{
        /// The name of this driver
        std::string GetDriverName() const override;

        /// Establish communications with the device
        /// @return true if the device was successfully setup
        bool Initialise() override;
        /// @copydoc IQKDDevice::GetAddress
        URI GetAddress() const override;
        /// @copydoc IQKDDevice::GetSessionController
        ISessionController*GetSessionController() override;
        /// @copydoc IQKDDevice::GetKeyPublisher
        IKeyPublisher*GetKeyPublisher() override;
        /// @copydoc IQKDDevice::GetDeviceDetails
        remote::Device GetDeviceDetails() override;
        /// @copydoc IQKDDevice::GetStats
        std::vector<stats::StatCollection*> GetStats() override;
        /// @}
    protected: // members
        /// What this driver is called
        static const std::string DriverName;

        /// For processing the incomming data
        class ProcessingChain;
        /// For processing the incomming data
        std::unique_ptr<ProcessingChain> processing;
        // our session controller
        std::unique_ptr<session::SessionController> sessionController;
        /// The driver which detects the photons
        std::shared_ptr<UsbTagger> driver;

    };
}
