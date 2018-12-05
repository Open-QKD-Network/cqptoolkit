/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/5/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#pragma once
#include "Networking/networking_export.h"
#include "Algorithms/Util/Strings.h"
#include <string>
#include "Algorithms/Util/Strings.h"

namespace cqp
{
    namespace tunnels
    {
        /// Predefined strings for tunnel communication
        namespace Names
        {
#if !defined(DOXYGEN)
    NAMEDSTRING(TunnelID);
    NAMEDSTRING(KeyName);
    NAMEDSTRING(IV);
    NAMEDSTRING(CipherData);
    NAMEDSTRING(LocalDetails);
    NAMEDSTRING(RemoteDetails);
    NAMEDSTRING(CompletedDetails);
    NAMEDSTRING(RequestedDetails);
    NAMEDSTRING(Result);

    NAMEDSTRING(Address);
    NAMEDSTRING(Port);

    NAMEDSTRING(Command);
    NAMEDSTRING(Response);
    NAMEDSTRING(ITunnelManager);
    NAMEDSTRING(CreateTunnel);
    NAMEDSTRING(CompleteTunnel);
    NAMEDSTRING(CloseTunnel);
    NAMEDSTRING(CompleteCloseTunnel);
    NAMEDSTRING(Upgrade);
#endif
        }

        namespace EndpointDetailsNames
        {
#if !defined(DOXYGEN)
    NAMEDSTRING(name);
    NAMEDSTRING(tunnelId);
    NAMEDSTRING(farSideController);
    NAMEDSTRING(farSideControllerId);
    NAMEDSTRING(active);
    NAMEDSTRING(keyDeviceId);
    NAMEDSTRING(encryptionMethod);
    NAMEDSTRING(clientDataPort);
    NAMEDSTRING(encryptedDataChannel);
    NAMEDSTRING(nodeIndex);
#endif
        }

        namespace ControllerDetailsNames
        {
#if !defined(DOXYGEN)
    NAMEDSTRING(name);
    NAMEDSTRING(address);
    NAMEDSTRING(port);
    NAMEDSTRING(devices);
    NAMEDSTRING(tunnels);
    NAMEDSTRING(id);
    NAMEDSTRING(connectionAddress);
#endif
        }

        namespace CypherSuiteNames
        {
#if !defined(DOXYGEN)
    NAMEDSTRING(keyExchangeAlgorithm);
    NAMEDSTRING(bultEncryptionAlgorithm);
    NAMEDSTRING(macAlgorithm);
    NAMEDSTRING(randomFunction);
#endif
        }
    }
}
