/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/5/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "CQPToolkit/Datatypes/Base.h"
#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Tunnels/Constants.h"
#include "CQPToolkit/Util/UUID.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <limits>
#include <chrono>

namespace cqp
{
    namespace tunnels
    {
        /// string constants for devices types
        namespace DeviceTypes
        {
#if !defined(DOXYGEN)
    NAMEDSTRING(eth);
    NAMEDSTRING(tun);   // raw IP packets
    NAMEDSTRING(tap);   // raw ethernet packets
    NAMEDSTRING(tcp);
    NAMEDSTRING(tcpsrv);
    NAMEDSTRING(udp);
    NAMEDSTRING(clavis2);
    NAMEDSTRING(crypto);
#endif
        }

    } // namespace tunnels
} // namespace cqp
