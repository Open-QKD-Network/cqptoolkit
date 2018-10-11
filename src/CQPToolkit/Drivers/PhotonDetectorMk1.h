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
#include <string>                                         // for string
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"  // for IPhotonEven...
#include "CQPToolkit/Interfaces/IQKDDevice.h"             // for IQKDDevice
#include "Util/URI.h"                                     // for URI
#include "CQPToolkit/Util/Provider.h"

namespace cqp
{
    class Serial;
    class UsbTagger;

    /// Processes detections from the low power detector
    /// @todo come up with a name for this device
    class CQPTOOLKIT_EXPORT PhotonDetectorMk1 : public virtual IQKDDevice, public Provider<IDetectionEventCallback>
    {
    public:
        /**
         * @brief PhotonDetectorMk1
         * Constructor which opens a device
        //         * @param cmdPortName Serial port name for control
         */
        explicit PhotonDetectorMk1(const std::string& cmdPortName);

        /**
         * @brief PhotonDetectorMk1
         * Open a device which uses both usb and serial
         * @param usbDev usb data transfer device
         * @param serialDev serial control device
         */
        PhotonDetectorMk1(UsbTagger* const usbDev, Serial* const serialDev);

        /**
         * Destructor
         */
        ~PhotonDetectorMk1() override;

        /// @name Device interface
        /// @{
        /// The name of this driver
        std::string GetDriverName() const override;

        /// @return true if the device is ready to use.
        virtual bool IsOpen() const;
        /// Create a connection to the device using the current parameters
        /// @return true if the operation was successful
        virtual bool Open();
        /// Disconnect from the device.
        /// @return true if disconnection completed without error
        /// @remarks The object must be return to a clean, disconnected state by this call
        ///         even if errors occour.
        virtual bool Close();

        /// Get the deescrition of the device which this instance manages
        /// @return The human readable name of the device
        virtual std::string GetDescription() const override;

        /// Esatablish communtications with the device
        /// @return true if the device was successfully detected
        bool Initialise() override;
        /// @}

        /// Begin the calibration steps for this device
        /// @return true if calibration was sucessful
        bool Calibrate();

        /// start collecting data
        /// @returns true on success
        bool BeginDetection();
        /// Stop collecting data
        /// @returns true on success
        bool EndDetection();
    protected:
        /// Device used for the C&C of the device
        Serial* commandDev = nullptr;
        /// Transfers the results using builk transfer
        UsbTagger* highSpeedDev = nullptr;
        /// OS Specific name for connecting to the port
        std::string serialPortName;

        // IQKDDevice interface
    public:
        URI GetAddress() const override;
    };
}

