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
        NAMEDSTRING(IKeyReceiver);
        NAMEDSTRING(parameters);
        NAMEDSTRING(frame);
        NAMEDSTRING(session);
        NAMEDSTRING(BeginSession);
        NAMEDSTRING(FrameComplete);
        NAMEDSTRING(CloseSession);
        NAMEDSTRING(ReturnCapabilities);
        NAMEDSTRING(InitSession);
    };

    namespace IKeyTransmitterCommands
    {
        NAMEDSTRING(IKeyTransmitter);
        NAMEDSTRING(parameters);
        NAMEDSTRING(frame);
        NAMEDSTRING(session);
        NAMEDSTRING(RequestCapabilities);
        NAMEDSTRING(RequestKeySession);
        NAMEDSTRING(ReadyToReceive);
        NAMEDSTRING(CloseSession);
    };

#endif
}
