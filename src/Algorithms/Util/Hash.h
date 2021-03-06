#pragma once
#include <cstdint>
#include <string>
#include "Algorithms/algorithms_export.h"

namespace cqp {

    /// Polynomial use for calculating the CRC with the CRCFddi function
    const uint32_t fddiPoly = 0x04c11db7;

    /// Calculate the CRC using the FDDI algorithm
    /// @see http://museotaranto.it/mvl/WebRes/ImageCoding/compress/crc.html
    /// @param[in] buf Data to calculate crc on
    /// @param[in] len Length of buf
    /// @return crc of input
    ALGORITHMS_EXPORT uint32_t CRCFddi(const void *buf, uint32_t len);

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
    template<class T>
    static inline uint64_t FNV1aHash(const T& value)
    {

        uint64_t hash = FNVOffset;
        for(auto byte : value)
        {
            // XOR the hash with the byte
            hash = hash ^ byte;
            hash = hash * FNVPrime;
        }

        return hash;
    }

    /**
     * @brief FNV1aHash
     * Perform a fast hash on the value. This is not suitable for security,
     * it is intended for fast string to collision resistant hashes for lookups.
     * @param start Start iterator of the list
     * @param end End iterator of the list
     * @return hash value
     */
    template<class Iter>
    static inline uint64_t FNV1aHash(Iter start, Iter end)
    {

        uint64_t hash = FNVOffset;
        while(start != end)
        {
            // XOR the hash with the byte
            hash = hash ^ *start;
            hash = hash * FNVPrime;
            start++;
        }

        return hash;
    }

} // namespace cqp


