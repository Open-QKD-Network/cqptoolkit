/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/8/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Strings.h"

namespace cqp
{
#if !defined(DOXYGEN)
    namespace IKeyReceiverCommands
    {
        CONSTSTRING IKeyReceiver = "IKeyReceiver";
        CONSTSTRING parameters = "parameters";
        CONSTSTRING frame = "frame";
        CONSTSTRING session = "session";
        CONSTSTRING BeginSession = "BeginSession";
        CONSTSTRING FrameComplete = "FrameComplete";
        CONSTSTRING CloseSession = "CloseSession";
        CONSTSTRING ReturnCapabilities = "ReturnCapabilities";
        CONSTSTRING InitSession = "InitSession";
    };

    namespace IKeyTransmitterCommands
    {
        CONSTSTRING IKeyTransmitter = "IKeyTransmitter";
        CONSTSTRING parameters = "parameters";
        CONSTSTRING frame = "frame";
        CONSTSTRING session = "session";
        CONSTSTRING RequestCapabilities = "RequestCapabilities";
        CONSTSTRING RequestKeySession = "RequestKeySession";
        CONSTSTRING ReadyToReceive = "ReadyToReceive";
        CONSTSTRING CloseSession = "CloseSession";
    };

#endif
}
