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
#include <string>
#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Util/URI.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include <memory>

namespace cqp
{

    class ISessionController;

    namespace remote
    {
        class Device;
    }

    /// A generic device driver which can communicate or simulate a piece of hardware.
    class CQPTOOLKIT_EXPORT IQKDDevice
    {
    public:
        /// Returns the user readable name of the driver
        /// @returns the user readable name of the driver
        virtual std::string GetDriverName() const = 0;

        /**
         * @brief GetAddress
         * @return Returns a URL which describes how to connect to the device
         */
        virtual URI GetAddress() const = 0;

        /// Get the description of the device which this instance manages
        /// @return The human readable name of the device
        virtual std::string GetDescription() const = 0;

        /// Establish communications with the device
        /// @return true if the device was successfully detected
        virtual bool Initialise() = 0;

        /**
         * @brief GetSessionController
         * @return The session controller which will manage the devices
         */
        virtual ISessionController* GetSessionController() = 0;

        /**
         * @brief GetDeviceDetails
         * @return Details for registration
         */
        virtual remote::Device GetDeviceDetails() = 0;

        /// virtual destructor
        virtual ~IQKDDevice() = default;

        /// URI parameter names
        struct Parmeters
        {
            /// The name of the switch port parameter
            static NAMEDSTRING(switchPort);
            /// The name of the side parameter
            static NAMEDSTRING(side);
            /// The name of the switch name parameter
            static NAMEDSTRING(switchName);
            /// The name of the key size parameter
            static NAMEDSTRING(keybytes);
            /// possible values for the side parameter
            struct SideValues
            {
                /// side is alice
                static NAMEDSTRING(alice);
                /// side is bob
                static NAMEDSTRING(bob);
                /// side is any
                static NAMEDSTRING(any);
            };
        };
    };
}

