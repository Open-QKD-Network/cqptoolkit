/*!
* @file
* @brief Alignment
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Alignment.h"
#include <climits>

namespace cqp
{
    namespace align
    {

        void Alignment::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {

        }

        void Alignment::Disconnect()
        {
            seq = 0;
        }

        Alignment::Alignment() = default;

        void Alignment::SendResults(const QubitList& emissions, double securityParameter)
        {
            if(HaveListener())
            {
                auto siftedData = std::make_unique<JaggedDataBlock>();
                // calculate the number of bits in the current system.
                constexpr uint8_t bitsPerValue = sizeof(DataBlock::value_type) * CHAR_BIT;
                siftedData->reserve(emissions.size() / bitsPerValue);

                JaggedDataBlock::value_type value = 0;
                uint_least8_t offset = 0;

                for(const auto& qubit : emissions)
                {
                    value |= QubitHelper::BitValue(qubit) << offset;
                    // move to the next bit
                    ++offset;

                    if(offset == bitsPerValue)
                    {
                        // we have filled up a word, add it to the output and reset
                        siftedData->push_back(value);
                        siftedData->bitsInLastByte = bitsPerValue;
                        value = 0;
                        offset = 0;
                    } // if
                }

                if(offset != 0)
                {
                    // There weren't enough bits to completely fill the last word,
                    // Add the remaining, the mask will show which bits are valid
                    siftedData->push_back(value);
                    siftedData->bitsInLastByte = offset;
                } // if(offset != 0)

                Emit(&ISiftedCallback::OnSifted, seq, securityParameter, move(siftedData));
                seq++;
            }
        }

    } // namespace align
} // namespace cqp
