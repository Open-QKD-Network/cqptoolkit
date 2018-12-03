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
#include "Algorithms/algorithms_export.h"
#include <vector>
#include <limits>
#include <cstdint>
#include <array>

namespace cqp
{

    /// Standard list of integers for general use.
    using IntList = std::vector<int>;

    /// Definition of a generic/opaque block of data.
    using DataBlock = std::vector<uint8_t>;

    /**
     * @brief The JaggedDataBlock class
     * An array of bytes where the last byte may contain an incomplete byte
     */
    class ALGORITHMS_EXPORT JaggedDataBlock : public DataBlock
    {
    public:
        /// The number of valid bits in the final byte
        /// 0 and 8 are equivalent
        uint8_t bitsInLastByte = 0;

        /**
         * @brief operator ==
         * @param right
         * @return true if the two objects are identical
         */
        bool operator==(const JaggedDataBlock& right) const
        {
            return static_cast<DataBlock>(right) == static_cast<DataBlock>(*this)
                   && this->bitsInLastByte == right.bitsInLastByte;
        }

        /**
         * @brief operator !=
         * @param right
         * @return true if the two objects are not identical
         */
        bool operator!=(const JaggedDataBlock& right) const
        {
            return ! (*this==right);
        }

        /**
         * @brief NumBits
         * @return number of bits stored
         */
        size_t NumBits() const
        {
            size_t result = size() * std::numeric_limits<uint8_t>::digits;

            if(bitsInLastByte != 0)
            {
                // take off the number of invalid bits
                result -= std::numeric_limits<uint8_t>::digits - bitsInLastByte;
            }
            return result;
        }

        /**
         * @brief LastByteComplete
         * @return  true if all bits in the last byte are valid
         */
        bool LastByteComplete() const
        {
            return bitsInLastByte == 0 || bitsInLastByte == std::numeric_limits<uint8_t>::digits;
        }
    };

    /// A single byte
    using Byte = unsigned char;

    /// Used for key negotiation to identify a portion of a key which is being exchanged
    using SequenceNumber = unsigned long;
    /// Default value for the frame ID
    const SequenceNumber nullSequenceNumber = 0;

} // namespace cqp


#if defined(__GNUC__) && __GNUC__ < 7
struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};
#endif

namespace std
{
    /**
     * Templated hash algorithm for arrays, allowing them to be used in unordered_map's
     */
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        /// array type to hash
        typedef array<T, N> argument_type;
        /// Hash type
        typedef size_t result_type;

        /**
         * @brief operator ()
         * @param a item to hash
         * @return hash of "a"
         */
        result_type operator()(const argument_type& a) const noexcept
        {
            hash<T> hasher;
            result_type h = 0;
            for (result_type i = 0; i < N; ++i)
            {
                h = h * 31 + hasher(a[i]);
            }
            return h;
        }
    };

} // namespace std

