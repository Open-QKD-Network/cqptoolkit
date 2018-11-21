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
#include <bits/stdint-uintn.h>                       // for uint16_t, uint8_t
#include <string>                                    // for string
#include "CQPToolkit/Drivers/Serial.h"               // for Serial, Serial::...
#include "CQPToolkit/Drivers/Usb.h"                  // for Usb
#include "CQPToolkit/Interfaces/IPhotonGenerator.h"  // for IPhotonGenerator
#include "CQPToolkit/Interfaces/IQKDDevice.h"        // for IQKDDevice
#include "QKDInterfaces/Site.pb.h"                   // for Device
#include "Util/URI.h"                                // for URI

namespace cqp
{
    class IRandom;
    class ISessionController;

    /// The storage for available drivers
    class CQPTOOLKIT_EXPORT LEDAliceMk1 : public Usb, public virtual IQKDDevice, public virtual IPhotonGenerator
    {
    public:
        /**
         * @brief LEDAliceMk1
         * Default constructor
         * @param randomSource
         */
        explicit LEDAliceMk1(IRandom* randomSource);

        /**
         * @brief LEDAliceMk1
         * Constructor
         * @param portName Serial port for configuring the device
         * @param description User readable device description
         */
        LEDAliceMk1(const std::string& portName, const std::string& description);

        virtual ~LEDAliceMk1() {}
        /// @name Device  realisation
        ///@{
        /// @copydoc IQKDDevice::GetDriverName
        virtual std::string GetDriverName() const override;

        /// @copydoc Usb::IsOpen
        virtual bool IsOpen() const override;
        /// @copydoc Usb::Open
        virtual bool Open() override;


        /// @copydoc Usb::Open
        /// @param portName Serial port for configuring the device
        /// @param description User readable device description
        virtual bool Open(const std::string& portName, const std::string& description);
        /// @copydoc Usb::Close
        bool Close() override;
        /// @copydoc IQKDDevice::Initialise
        virtual bool Initialise() override;

        /// @copydoc cqp::IQKDDevice::GetDeviceDetails
        remote::Device GetDeviceDetails() override;
        ///@}

        /// @name IPhotonGenerator Interface realisation
        ///@{

        /// @copydoc IPhotonGenerator::Fire
        void Fire() override
        {
            Fire(photonsPerBurst);
        }
        ///@}

        /// @copydoc IPhotonGenerator::Fire
        void Fire(unsigned long numQubits);
    protected:
        /// make the hidden function protected
        using Usb::Open;
        /// The serial port used to configure the device
        Serial configPort;

        /// device name for serial port
        std::string myPortName = "";
        /// speed used to communicate with the device
        Serial::BaudRate myBaudRate = Serial::BaudRate::B_9600;

        /// the usb vendor id of the device
        const uint16_t UsbVID = 0x221a;
        /// the usb product id of the device
        const uint16_t UsbPID = 0x100;
        /// usb end point to use
        const int UsbEndpoint = 0x02;
        /// how many bits are stransmitted for each qubit
        const uint8_t bitsPerQubit = 2;
        /// Stuff to send over serial to get Alice to send at 10MHz with a reasonable pulse width (7ns?)
        const unsigned char divEndpoint = 0x46;
        /// configuration command
        const unsigned char div10Mhz = 19;
        /// end marker for configuration command
        const unsigned char commandEnd = '$';
        /// ?
        const unsigned char delEndPoint = 0x50;
        /// ?
        const unsigned char del7ns = 28;
        /// Source for random qubits
        IRandom* randomness = nullptr;

        /// number of photons to send each frame
        unsigned long photonsPerBurst = 1024;

        // IPhotonGenerator interface
    public:
        void StartFrame() override;
        void EndFrame() override;

        // IQKDDevice interface
    public:
        URI GetAddress() const override;
        std::string GetDescription() const override;

        // IQKDDevice interface
    public:
        ISessionController* GetSessionController() override;
    };
}
