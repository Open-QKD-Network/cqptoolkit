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
#include "CQPToolkit/Util/UUID.h"
#include "CQPToolkit/Datatypes/DetectionReport.h"

namespace cqp
{

    /// Used for key negotiation to identify the current conversation
    using SessionID = UUID;

    struct CQPTOOLKIT_EXPORT SystemParameters {
        PicoSeconds frameWidth {0};
        PicoSeconds slotWidth {0};
        PicoSeconds pulseWidth {0};
    };
} // namespace cqp
