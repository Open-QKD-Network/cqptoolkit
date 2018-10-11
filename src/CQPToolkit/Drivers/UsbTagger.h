/*!
 * @file
 * @brief CQP Toolkit - Usb Tagger
 *
 * @copyright Copyright (C) University of Bristol 2016
 *    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
 * @date 08 Feb 2016
 * @author Richard Collins <richard.collins@bristol.ac.uk>
 */
#pragma once
#include <bits/stdint-uintn.h>                 // for uint16_t, uint8_t
#include <string>                              // for string
#include <vector>                              // for vector
#include "CQPToolkit/Drivers/Usb.h"            // for Usb
#include "CQPToolkit/Interfaces/IQKDDevice.h"  // for IQKDDevice
#include "QKDInterfaces/Site.pb.h"             // for Device
#include "Util/URI.h"                          // for URI
struct libusb_transfer;

namespace cqp
{
    class ISessionController;
    class UsbTagger;
    /// A collection of USB Taggers
    using UsbTaggerList = std::vector<UsbTagger*>;

    /// Class for controlling the "RWN 11" time tagger and coincedence counter built at UOB.
    class CQPTOOLKIT_EXPORT UsbTagger : public Usb, public virtual IQKDDevice
    {
    public:
        // provide access to all parent Open methods
        using Usb::Open;
        /// Open the connection using the current parameters
        /// @return true on success
        virtual bool Open() override;

        /// Perform device detection
        /// @param[out] results found devices are added to this list
        /// @param[in] firstOnly Stop after first detection
        static void DetectFunc(UsbTaggerList& results, bool firstOnly);

        ///@{
        /// @name IQKDDevice interface

        /// Access the name of the driver
        /// @return The human readable name of the driver
        std::string GetDriverName() const override;

        /// Esatablish communtications with the device
        /// @return true if the device was successfully detected
        virtual bool Initialise() override;
        /// @copydoc IQKDDevice::GetAddress
        URI GetAddress() const override;
        /// @copydoc IQKDDevice::GetDescription
        std::string GetDescription() const override;
        /// @copydoc IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;
        /// @copydoc IQKDDevice::GetDeviceDetails
        remote::Device GetDeviceDetails() override;
        ///@}

        /// Default constructor
        UsbTagger();
        /// Default distructor
        virtual ~UsbTagger() override;
    protected:
        /// The vendor id of this device
        const uint16_t usbVid = 0x221A;
        /// The product id of the device
        const uint16_t usbPid = 0x0100;
        /// the request id for the standard builk transfer read
        const uint8_t BulkReadRequest = 0x82;
        /// Called by Usb::ReadCallback() when a read has completed
        /// @param transfer Details of the transfer
        void ReadCallback(libusb_transfer* transfer) override;

    };

}
