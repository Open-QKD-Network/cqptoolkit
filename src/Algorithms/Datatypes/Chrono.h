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
    /// A definition of time for use with time tagging
    using FemtoSeconds = std::chrono::duration<uint64_t, std::femto>;
    /// A definition of time for use with time tagging
    using AttoSeconds = std::chrono::duration<uint64_t, std::atto>;

    /// signed duration value to allow time to go in both directions
    using PicoSecondOffset = std::chrono::duration<int64_t, std::pico>;
    /// A definition of time for use with time tagging
    using FemtoSecondOffset = std::chrono::duration<int64_t, std::femto>;
    /// signed duration value to allow time to go in both directions
    using AttoSecondOffset = std::chrono::duration<int64_t, std::atto>;

    /// floating point number of seconds
    using SecondsDouble = std::chrono::duration<double, std::chrono::seconds::period>;
} // namespace cqp
