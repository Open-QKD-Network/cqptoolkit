/*!
* @file
* @brief Interface
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>
#include <vector>

namespace cqp
{
    namespace net
    {

        /**
         * @brief The Interface class
         * Utilities for network interfaces
         */
        class Interface
        {
        public:
            /**
             * @brief Interface
             * Constructor
             */
            Interface() = default;

            /// Kind of address
            enum InterfaceType { Any, Private };

            /**
             * @brief GetInterfaceNames
             * @param interfaceType
             * @return A list of interface names matching the address kind
             * eg: "eth0"
             */
            static std::vector<std::string> GetInterfaceNames(InterfaceType interfaceType = Any);

            /**
             * @brief GetInterfaceBroadcast
             * @param interfaceType
             * @return A list of broadcast ip addresses matching the address kind
             * eg: "192.168.1.255"
             */
            static std::vector<std::string> GetInterfaceBroadcast(InterfaceType interfaceType = Any);
        };
    } // namespace net
} // namespace cqp


