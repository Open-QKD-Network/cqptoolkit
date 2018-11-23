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
#include "Base.h"
#include "Framing.h"
#include "CQPToolkit/Util/Logger.h"
#include <iomanip>

namespace cqp
{

    /// A sequence number for identifying individual keys
    using KeyID = uint64_t;

    /// A pre shared key type
    class CQPTOOLKIT_EXPORT PSK : public DataBlock
    {
    public:
        /// Default constructor
        PSK() {}
        /// Construct a PSK from a data block
        /// @param a source data
        PSK(const DataBlock& a) : DataBlock(a) {}
        /// Provide access to the parent constructor
        using DataBlock::DataBlock;

        /**
         * XOr two lists
         * @tparam T A sequence of bytes to xor with
         * @param right the list to xor with this object
         * @return this object
         */
        template<typename T>
        PSK &operator ^=(const T& right) noexcept
        {
            if(size() == right.size())
            {
                // Tests were done with optimising this by using the full word width,
                // no measurable improvement was found when running in release mode,
                // for readability it is left to the compiler to optimise
                /*
                 * const uint8_t bytesInWord = sizeof(size_t);
                 *
                 * if(size() % bytesInWord == 0)
                 * {
                 *     // do multi byte xor
                 *     const size_t maxIndex = size() / bytesInWord;
                 *     const size_t* rightBig = reinterpret_cast<const size_t*>(&right[0]);
                 *     size_t* resultBig = reinterpret_cast<size_t*>(&(*this)[0]);
                 *
                 *     for(size_t index = 0; index < maxIndex; index++)
                 *     {
                 *         resultBig[index] ^= rightBig[index];
                 *     }
                 * }
                 */
                // do byte by byte
                for(size_t index = 0; index < size(); index++)
                {
                    (*this)[index] ^= static_cast<unsigned char>(right[index]);
                }

            }
            else
            {
                LOGERROR("Key lengths don't match");
            }

            return *this;
        }

        /**
         * @brief ToString
         * @return The bytes in hex
         */
        std::string ToString() const
        {
            using namespace std;
            std::stringstream out;
            for(auto byte : *this)
            {
                // the "+" forces the byte to be read as a number
                out << setw(2) << setfill('0') << uppercase << hex << +byte;
            }
            return out.str();
        } // ToString
    }; // class PSK

    /// Initialisation vector type for encryption algorithms
    using IV = DataBlock;

    /// A list of keys
    using KeyList = std::vector<PSK>;

    // with associated key id
    /*class CQPTOOLKIT_EXPORT KeyList : public std::vector<PSK>
    {
    public:
        /// returns the sequence number for a specific element
        /// @param[in] index 0 based offset for the element
        /// @return The key id of the key element
        KeyID GetSeqNum(const size_t index) const
        {
            return firstSequenceNumber + index;
        }

        /// Change the sequence number for the first item in the list
        /// @param n The key id for the first element
        void SetFirstSeqNum(const KeyID n)
        {
            firstSequenceNumber = n;
        }
    private:
        /// Stores the id for the first item in the list, the number is sequencial for all elements
        KeyID firstSequenceNumber = {};
    };*/

}
