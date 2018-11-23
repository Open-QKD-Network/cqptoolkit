/*!
* @file
* @brief CQP Toolkit - QKD specific parameters
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Platform.h"

namespace cqp
{
    namespace Parameters
    {
        /// The default number of qubits to send within each frame
        const uint32_t DefaultFrameSize = 1024;
        /// The number of qubits to send within each frame
        const std::string FrameSize = "FrameSize";
        /// If the number of frames is set to infinite, the session will continue until it is stopped
        const uint32_t InfiniteNumberFrames = 0;
        /// The default number of frames to process in a given session.
        const uint32_t DefaultNumberFrames = 1024;
        /// The number of frames to process before closing the session
        const std::string NumberFrames = "NumberFrames";

        /// a parameter which specifies the unique id of the quantum channel endpoint which which this device is connected to.
        const std::string AliceEndpointIdentifier = "AliceEndpointIdentifier";
        /// a parameter which specifies the unique id of the quantum channel endpoint which which this device is connected to.
        const std::string BobEndpointIdentifier = "BobEndpointIdentifier";

    }
}
