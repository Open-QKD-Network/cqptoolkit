/*!
* @file
* @brief Device
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 10/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>

namespace cqp
{
    namespace net
    {

        /**
         * @brief The Device class
         * Handler class for network hardware
         */
        class Device
        {
        public:
            /**
             * @brief Device
             * Constructor
             */
            Device() = default;

            /**
             * @brief SetAddress
             * Assign an ip address to the device
             * @param devName system identifier for the device
             * @param address the IP address to assign
             *  version 4 or 6 are supported
             * @param netmask The Netmask to assign
             *  version 4 or 6 are supported
             * @return true on success
             */
            static bool SetAddress(const std::string& devName, const std::string& address, const std::string& netmask = "");

            /**
             * @brief Up
             * @param devName The system identifier for the device
             * @return true on success
             */
            static bool Up(const std::string& devName);

            /**
             * @brief Down
             * @param devName The system identifier for the device
             * @return true on success
             */
            static bool Down(const std::string& devName);


            /**
             * @brief SetRunState
             * @param devName The system identifier for the device
             * @param up Should the device be brought up or down
             * @return true on success
             */
            static bool SetRunState(const std::string& devName, bool up);
        };

    } // namespace net
} // namespace cqp


