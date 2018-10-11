/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

// prototypes to limit the build tim dependencies
namespace cqp
{
    class UUID;
    class URI;

    namespace remote
    {
        namespace tunnels
        {
            class Tunnel;
            class TunnelEndDetails;
            class NetworkDevice;
            class ControllerDetails;
        }
    }
}
