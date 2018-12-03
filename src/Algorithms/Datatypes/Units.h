/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
/**
 * @brief postfix operator
 * Scale the number to Kibibytes (multiples of 1024)
 * Example usage: @code bytes = 12_KiB @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _KiB(unsigned long long count) noexcept
{
    return count * 1024;
}
/**
 * @brief postfix operator
 * Scale the number to Megibytes (multiples of 1024)
 * Example usage: @code bytes = 12_MiB @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _MiB(unsigned long long count) noexcept
{
    return count * 1024 * 1024;
}

/**
 * @brief postfix operator
 * Scale the number to Gigibytes (multiples of 1024)
 * Example usage: @code bytes = 12_GiB @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _GiB(unsigned long long count) noexcept
{
    return count * 1024 * 1024 * 1024;
}

/**
 * @brief postfix operator
 * Scale the number to Kilobytes (multiples of 1000)
 * For multiples of 1024, use _KiB
 * Example usage: @code bytes = 12_kb @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _kb(unsigned long long count) noexcept
{
    return count * 1000;
}

/**
 * @brief postfix operator
 * Scale the number to Megabytes (multiples of 1000)
 * For multiples of 1024, use _MiB
 * Example usage: @code bytes = 12_mb @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _mb(unsigned long long count) noexcept
{
    return count * 1000 * 1000;
}

/**
 * @brief postfix operator
 * Scale the number to Gibibytes (multiples of 1000)
 * For multiples of 1024, use _XiB
 * Example usage: @code bytes = 12_gb @endcode
 * @param count the number to raise to the postfix
 * @return value scaled to the meaning of the postfix
 */
constexpr unsigned long long operator "" _gb(unsigned long long count) noexcept
{
    return count * 1000 * 1000 * 1000;
}
