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
#include "Util.h"
#include "CQPToolkit/Util/Logger.h"
#include <iostream>
#include <iomanip>
#include <locale>
#include <cstdlib>

namespace cqp
{
    /// Holds pre-calculated crc values
    static unsigned long crc32_table[256] = {};

    /// populate the crc32_table
    void ComputeFddiTable()
    {
        uint i, j;
        uint32_t c;

        for (i = 0; i < 256; ++i)
        {
            for (c = i << 24, j = 8; j > 0; --j)
            {
                c = c & 0x80000000 ? (c << 1) ^ fddiPoly : (c << 1);
            }
            crc32_table[i] = c;
        }
    }

    /**
     * @brief CRCFddi
     * @param buf Data to generate checksum for
     * @param len Length of data
     * @return checksum
     */
    uint32_t CRCFddi(const void *buf, uint32_t len)
    {
        unsigned char const *p;
        uint32_t  crc;

        if (!crc32_table[1])    /* if not already done, */
        {
            ComputeFddiTable();   /* build table */
        }

        crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
        for (p = reinterpret_cast<uint8_t const *>(buf); len > 0; ++p, --len)
        {
            crc = static_cast<uint32_t>((crc << 8) ^ crc32_table[(crc >> 24) ^ *p]);
        }

        crc = ~crc;            /* transmit complement, per CRC-32 spec */
        /* DANGER: this was added to make the CRC match the clavis 2 crc
        * its correctness on little and/or big endian is not confirmed */
        uint32_t result = 0;
        result |= (crc << 24) & 0xFF000000;
        result |= (crc <<  8) & 0x00FF0000;
        result |= (crc >>  8) & 0x0000FF00;
        result |= (crc >> 24) & 0x000000FF;
        return result;
    }

    std::string Join(const std::vector<std::string>& strings, const std::string& delimiter)
    {
        using namespace std;
        string result;
        bool useDelim = false;
        const bool haveDeleim = !delimiter.empty();

        for(const auto& element : strings)
        {
            if(haveDeleim)
            {
                if(useDelim)
                {
                    result.append(delimiter);
                }
                else
                {
                    useDelim = true;
                }
            }

            result.append(element);
        }

        return result;
    }

#if !defined(memset_s)
#if !defined(DOXYGEN)
    #include <cerrno>
#endif

    /// Copies the value ch (after conversion to unsigned char as if by (unsigned char)ch) into each of the first count characters of the object pointed to by dest.
    /// @see http://en.cppreference.com/w/c/string/byte/memset
    /// @details
    /// memset may be optimized away (under the as-if rules) if the object modified by this function is not accessed again for the rest of its lifetime (e.g. gcc bug 8537). For that reason, this function cannot be used to scrub memory (e.g. to fill an array that stored a password with zeroes).
    /// This optimization is prohibited for memset_s: it is guaranteed to perform the memory write. Third-party solutions for that include FreeBSD explicit_bzero or Microsoft SecureZeroMemory.
    /// @returns zero on success, non-zero on error. Also on error, if dest is not a null pointer and destsz is valid, writes destsz fill bytes ch to the destination array.
    /// @param dest Memory region to change
    /// @param destsz size of destination
    /// @param ch value to write to destination
    /// @param count number of bytes to write
    int memset_s(void *dest, size_t destsz, char ch, size_t count)
    {
        if (dest == nullptr) return EINVAL;
        if (destsz > SIZE_MAX) return EINVAL;
        if (count > destsz) return EINVAL;

        volatile char *p = static_cast<volatile char*>(dest);
        while (destsz-- && count--)
        {
            *p++ = ch;
        }

        return 0;
    }
#endif

    void SecureErase(void* data, unsigned int length)
    {
        using namespace std;
        memset_s(data, length, 0, length);
    }

    bool StrEqualI(const std::string& left, const std::string& right)
    {
        bool result = left.size() == right.size();
        if(result)
        {
            for(unsigned i = 0; i < left.length(); i++)
            {
                if(std::tolower(left[i]) != std::tolower(right[i]))
                {
                    result = false;
                    break; // for
                }
            }
        }

        return result;
    }

    std::string GetEnvironmentVar(const std::string& key)
    {
        return std::string(getenv(key.c_str()));
    }

    void SplitString(const std::string& value, std::vector<std::string>& dest, const std::string& seperator, size_t startAt)
    {
        using std::string;
        size_t from = startAt;
        size_t splitPoint = 0;
        splitPoint = value.find(seperator, from);
        while(splitPoint != string::npos)
        {
            string iface = value.substr(from, splitPoint - from);
            if(!iface.empty())
            {
                dest.push_back(iface);
            }
            from = splitPoint + 1;
            splitPoint = value.find(seperator, from);
        }

        if(from < value.size())
        {
            dest.push_back(value.substr(from));
        }
    }

    void SplitString(const std::string& value, std::unordered_set<std::string>& dest, const std::string& seperator, size_t startAt)
    {
        std::vector<std::string> temp;
        SplitString(value, temp, seperator, startAt);
        for(auto& element: temp)
        {
            dest.insert(element);
        }
    }

    std::vector<uint8_t> HexToBytes(const std::string& hex)
    {
        std::vector<uint8_t> bytes;
        if(hex.length() % 2 == 0)
        {
            bytes.reserve(hex.length() / 2);

            for (unsigned int i = 0; i < hex.length(); i += 2)
            {
                std::string byteString = hex.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(strtoul(byteString.c_str(), nullptr, 16));
                bytes.push_back(byte);
            }
        }
        else
        {
            LOGERROR("Invalid length for hex bytes");
        }
        return bytes;
    }

    void ToDictionary(const std::string& delimited, std::map<std::string, std::string>& dictionary, char pairSeperator, char keyValueSep)
    {
        using namespace std;
        stringstream input(delimited);
        string param;
        while(getline(input, param, pairSeperator))
        {
            size_t equalPos = param.find(keyValueSep);
            std::string value;
            if(equalPos != std::string::npos)
            {
                value = param.substr(equalPos + 1);
            }
            dictionary[param.substr(0, equalPos)] = value;
        }
    }

}
