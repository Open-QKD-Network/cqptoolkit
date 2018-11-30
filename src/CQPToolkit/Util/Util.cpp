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
#include "CQPAlgorithms/Logging/Logger.h"
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

}
