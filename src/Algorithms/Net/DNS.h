/*!
* @file
* @brief DNS
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/algorithms_export.h"
#include <string>
#include <vector>

namespace cqp
{

    namespace net
    {

        struct IPAddress;

        /**
         * @brief GetHostname
         * @param fqdn if true return the fully qualified host name
         * @return The hostname of this machine
         */
        ALGORITHMS_EXPORT std::string GetHostname(bool fqdn = true) noexcept;

        /**
         * @brief GetHostIPs
         * @return A list of ip addresses for this host
         */
        ALGORITHMS_EXPORT std::vector<IPAddress> GetHostIPs() noexcept;

        /**
         * @brief ResolveAddress
         * @param hostname The hostname to resolve
         * @param[out] ip The first result from the resolution
         * @param preferIPv6 If this is true, the ipv6 ip address will be returned if there is one
         *  otherwise the v4 address will be returned, unless there is only a v6 address
         * @return true on success
         */
        ALGORITHMS_EXPORT bool ResolveAddress(const std::string& hostname, IPAddress& ip, bool preferIPv6 = false) noexcept;

    } // namespace net
} // namespace cqp


