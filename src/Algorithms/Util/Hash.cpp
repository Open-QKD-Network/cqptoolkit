#include "Hash.h"

namespace cqp {

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


} // namespace cqp
