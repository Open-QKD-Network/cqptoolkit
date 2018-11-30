/*!
* @file
* @brief CQP Toolkit - Utilities
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16 May 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "CQPToolkit/cqptoolkit_export.h"
#include <climits>
#include <stdlib.h>
#include <vector>

#include <algorithm>
#include <cctype>
#include <locale>
#include <unordered_set>
#include <iomanip>
#include <map>

#pragma once

namespace cqp
{

    /// Polynomial use for calculating the CRC with the CRCFddi function
    const uint32_t fddiPoly = 0x04c11db7;

    /// Calculate the CRC using the FDDI algorithm
    /// @see http://museotaranto.it/mvl/WebRes/ImageCoding/compress/crc.html
    /// @param[in] buf Data to calculate crc on
    /// @param[in] len Length of buf
    /// @return crc of input
    CQPTOOLKIT_EXPORT uint32_t CRCFddi(const void *buf, uint32_t len);

    /// Clear a memory region such that it's contents cannot be recovered
    /// @param[in,out] data Address to start clearing
    /// @param[in] length Number of bytes to clear
    void SecureErase(void* data, unsigned int length);

    /// @copybrief SecureErase
    /// @tparam T The data type stored in the vector
    /// @param data data to clear
    template<typename T>
    void SecureErase(std::vector<T>& data)
    {
        SecureErase(data.data(), data.size() * sizeof(T));
    }


    /// initial value for the hash function
    constexpr uint64_t FNVOffset = 0xcbf29ce484222325;
    /// hash multiplier
    constexpr uint64_t FNVPrime = 0x100000001b3;

    /**
     * @brief FNV1aHash
     * Perform a fast hash on the value. This is not suitable for security,
     * it is intended for fast string to collision resistant hashes for lookups.
     * @param value string to hash
     * @return hash value
     */
    static inline uint64_t FNV1aHash(const std::string& value)
    {

        uint64_t hash = FNVOffset;
        for(auto byte : value)
        {
            // XOR the hash with the byte
            hash = hash ^ static_cast<unsigned char>(byte);
            hash = hash * FNVPrime;
        }

        return hash;
    }

}
