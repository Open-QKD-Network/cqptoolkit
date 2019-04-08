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
    CONSTSTRING TunnelID = "TunnelID";
    CONSTSTRING KeyName = "KeyName";
    CONSTSTRING IV = "IV";
    CONSTSTRING CipherData = "CipherData";
    CONSTSTRING LocalDetails = "LocalDetails";
    CONSTSTRING RemoteDetails = "RemoteDetails";
    CONSTSTRING CompletedDetails = "CompletedDetails";
    CONSTSTRING RequestedDetails = "RequestedDetails";
    CONSTSTRING Result = "Result";

    CONSTSTRING Address = "Address";
    CONSTSTRING Port = "Port";

    CONSTSTRING Command = "Command";
    CONSTSTRING Response = "Response";
    CONSTSTRING ITunnelManager = "ITunnelManager";
    CONSTSTRING CreateTunnel = "CreateTunnel";
    CONSTSTRING CompleteTunnel = "CompleteTunnel";
    CONSTSTRING CloseTunnel = "CloseTunnel";
    CONSTSTRING CompleteCloseTunnel = "CompleteCloseTunnel";
    CONSTSTRING Upgrade = "Upgrade";
#endif
        }

        namespace EndpointDetailsNames
        {
#if !defined(DOXYGEN)
    CONSTSTRING name = "name";
    CONSTSTRING tunnelId = "tunnelId";
    CONSTSTRING farSideController = "farSideController";
    CONSTSTRING farSideControllerId = "farSideControllerId";
    CONSTSTRING active = "active";
    CONSTSTRING keyDeviceId = "keyDeviceId";
    CONSTSTRING encryptionMethod = "encryptionMethod";
    CONSTSTRING clientDataPort = "clientDataPort";
    CONSTSTRING encryptedDataChannel = "encryptedDataChannel";
    CONSTSTRING nodeIndex = "nodeIndex";
#endif
        }

        namespace ControllerDetailsNames
        {
#if !defined(DOXYGEN)
    CONSTSTRING name = "name";
    CONSTSTRING address = "address";
    CONSTSTRING port = "port";
    CONSTSTRING devices = "devices";
    CONSTSTRING tunnels = "tunnels";
    CONSTSTRING id = "id";
    CONSTSTRING connectionAddress = "connectionAddress";
#endif
        }

        namespace CypherSuiteNames
        {
#if !defined(DOXYGEN)
    CONSTSTRING keyExchangeAlgorithm = "keyExchangeAlgorithm";
    CONSTSTRING bultEncryptionAlgorithm = "bultEncryptionAlgorithm";
    CONSTSTRING macAlgorithm = "macAlgorithm";
    CONSTSTRING randomFunction = "randomFunction";
#endif
        }
    }
}
