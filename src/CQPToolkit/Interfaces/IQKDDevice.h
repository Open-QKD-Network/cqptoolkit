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
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/URI.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "Algorithms/Util/Provider.h"
#include <memory>
#include "Algorithms/Util/IEvent.h"

namespace cqp
{

    class ISessionController;

    namespace remote
    {
        class Device;
        class SiteAgentReport;
        class DeviceConfig;
    }

    namespace stats
    {
        class IStatsReportCallback {
        public:
            virtual void StatsReport(const remote::SiteAgentReport& report) = 0;
            virtual ~IStatsReportCallback() = default;
        };

        using IStatsPublisher = IEvent<IStatsReportCallback>;

    }

    /// Manages Key callbacks
    /// @see Provider
    using IKeyPublisher = Provider<IKeyCallback>;

    /// A generic device driver which can communicate or simulate a piece of hardware.
    class CQPTOOLKIT_EXPORT IQKDDevice
    {
    public:
        /// Returns the user readable name of the driver
        /// @returns the user readable name of the driver
        virtual std::string GetDriverName() const = 0;

        /**
         * @brief GetAddress
         * The schema must identify the driver
         * The combination of host and port fields must uniquely identify the device on a system.
         * A `side` parameter must be included to denote the device type.
         * @return Returns a URL which describes how to connect to the device
         */
        virtual URI GetAddress() const = 0;

        /// Establish communications with the device and configure it for the specifed parameters
        /// Th parameters should be adjusted to reflect the actial values used where default/invalid values are provided
        /// @param[in,out] parameters The system settings to use when configuring the device.
        /// @return true if the device was successfully setup
        virtual bool Initialise(remote::DeviceConfig& parameters) = 0;

        /**
         * @brief GetSessionController
         * @return The session controller which will manage the devices
         */
        virtual ISessionController* GetSessionController() = 0;

        /**
         * @brief GetKeyPublisher
         * @return The interface which will issue callbacks when key is available
         */
        virtual IKeyPublisher* GetKeyPublisher() = 0;

        /**
         * @brief GetDeviceDetails
         * @return Details for registration
         */
        virtual remote::Device GetDeviceDetails() = 0;

        /**
         * @brief GetStatsPublisher
         * @return The producer of device statistics
         */
        virtual stats::IStatsPublisher* GetStatsPublisher() = 0;

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

