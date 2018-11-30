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
#include "CQPAlgorithms/Datatypes/UUID.h"
#include "CQPAlgorithms/Datatypes/Chrono.h"

namespace cqp
{

    /// Used for key negotiation to identify the current conversation
    using SessionID = UUID;

    /**
     * @brief The SystemParameters struct
     * Provides configuration parameters for the setup of frames and photon exchange
     */
    struct CQPALGORITHMS_EXPORT SystemParameters {
        /// The duration for a frame
        PicoSeconds frameWidth {0};
        /// The duration for one slot within a frame
        PicoSeconds slotWidth {0};
        /// The width of a single pulse in a slot
        PicoSeconds pulseWidth {0};
    };
} // namespace cqp
