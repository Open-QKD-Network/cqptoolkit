/*!
* @file
* @brief Chrono
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/5/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <chrono>

namespace cqp
{

    /// A definition of time for use with time tagging
    using PicoSeconds = std::chrono::duration<uint64_t, std::pico>;

    /// signed duration value to allow time to go in both directions
    using PicoSecondOffset = std::chrono::duration<int64_t, PicoSeconds::period>;

    /// floating point number of seconds
    using SecondsDouble = std::chrono::duration<double, std::chrono::seconds::period>;
} // namespace cqp
