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
#include "CQPToolkit/Util/Platform.h"
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

    /// Concatenate the strings optionally separating them with a delimiter
    /// @param strings The strings to concatenate
    /// @param delimiter Separator between each string
    /// @return The concatenated string.
    std::string Join(const std::vector<std::string>& strings, const std::string& delimiter = "");

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

    /**
     * @brief str2int
     * Provides a hash of a string which can be used in switch statements
     * @param str
     * @param h
     * @return hash of str
     */
    constexpr size_t str2int(const char* str, int h = 0)
    {
        return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ static_cast<size_t>(str[h]);
    }

    /**
     * @brief StrCompI
     * Compare two string, ignoring case
     * @param left
     * @param right
     * @return true if they match
     */
    CQPTOOLKIT_EXPORT bool StrEqualI(const std::string& left, const std::string& right);

    /**
     * @brief GetEnvironmentVar
     * @param key Name of environment variable
     * @return The value of the environment variable
     */
    CQPTOOLKIT_EXPORT std::string GetEnvironmentVar(const std::string& key);

    /**
     * @brief ltrim
     * Remove white space from the left of the string
     * @note thanks to https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
     * trim from start (in place)
     * @param s string to trim
     */
    static inline void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));
    }

    /**
     * @brief rtrim
     * Remove white space from the end of the string
     * @note thanks to https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
     * trim from start (in place)
     * @param s string to trim
     */
    static inline void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
        {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    /**
     * @brief trim
     * trim from both ends (in place)
     * @param s string to trim
     */
    static inline void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    /**
     * @brief SplitString
     * Tokenise the string
     * @param[in] value string to tokenise
     * @param[out] dest tokens
     * @param[in] separator How to separate the string
     * @param[in] startAt Start point of value
     */
    CQPTOOLKIT_EXPORT void SplitString(const std::string& value, std::vector<std::string>& dest, const std::string& separator, size_t startAt = 0);


    /**
     * @brief SplitString
     * Tokenise the string
     * @param[in] value string to tokenise
     * @param[out] dest tokens
     * @param[in] separator How to separate the string
     * @param[in] startAt Start point of value
     */
    CQPTOOLKIT_EXPORT void SplitString(const std::string& value, std::unordered_set<std::string>& dest, const std::string& separator, size_t startAt = 0);

    /**
     * @brief ToDictionary
     * Convert a delimited string with key value pairs into a dictionary
     * @param delimited string to separate, eg ``key1=abc;key2=def``
     * @param pairSeparator separator between key value pairs
     * @param keyValueSep separator between key and value
     * @param dictionary destination for results
     */
    CQPTOOLKIT_EXPORT void ToDictionary(const std::string& delimited, std::map<std::string, std::string>& dictionary,
                                        char pairSeparator, char keyValueSep = '=');

    /**
     * @brief ToLower
     * Convert a string to lower case
     * @param mixed
     * @return mixed in lower case
     */
    inline std::string ToLower(const std::string& mixed)
    {
        std::string result(mixed);
        std::transform(mixed.begin(), mixed.end(), result.begin(), ::tolower);
        return result;
    }

    /**
     * @brief HexToBytes
     * Read a string formatted in hex as raw bytes
     * @param hex
     * @return byte array
     */
    CQPTOOLKIT_EXPORT std::vector<uint8_t> HexToBytes(const std::string& hex);

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

    /**
     * @brief ToHexString
     * @tparam T The type of value
     * @param value Value to convert to hex
     * @return The value in uppercase hex, at least 2 characters wide, no prefix
     */
    template<typename T>
    std::string ToHexString(const T& value)
    {
        using namespace std;
        std::stringstream result;

        result << setw(2) << setfill('0') << uppercase << hex << +value;
        return result.str();
    }
}
