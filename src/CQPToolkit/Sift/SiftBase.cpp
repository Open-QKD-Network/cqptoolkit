/*!
* @file
* @brief SiftBase
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiftBase.h"

namespace cqp
{
    namespace sift
    {

        SiftBase::SiftBase(std::mutex& mutex, std::condition_variable& conditional):
            statesMutex{mutex}, statesCv{conditional}
        {

        }

        void SiftBase::SetDiscardedIntensities(std::set<Intensity> intensities)
        {
            discardedIntensities = intensities;
        }

        bool SiftBase::PackQubit(Qubit qubit, size_t index, const cqp::remote::BasisAnswers& answers,
                                 JaggedDataBlock& siftedData,
                                 uint_least8_t& offset, JaggedDataBlock::value_type& byteBuffer)
        {
            bool result = false;
            // test if this qubit is valid, ie the basis matched when they were compared.
            if(answers.answers(index) == true)
            {
                // pack the qubit if we're not using intensities or the intensity value is one we keep
                if(answers.intensity().empty() ||
                        discardedIntensities.find(answers.intensity(index)) == discardedIntensities.end())
                {
                    // we're using the qubit
                    result = true;
                    // shift the bit up to the next available slot and or it with the current value
                    byteBuffer |= static_cast<JaggedDataBlock::value_type>(QubitHelper::BitValue(qubit)) << offset;
                    // move to the next bit
                    ++offset;

                    // overflowing the byte, move to next byte
                    if(offset == bitsPerValue)
                    {
                        // we have filled up a word, add it to the output and reset
                        siftedData.push_back(byteBuffer);
                        siftedData.bitsInLastByte = bitsPerValue;
                        byteBuffer = 0;
                        offset = 0;
                    } // if overflow
                } // if intensity is kept
            } // if basis match

            return result;
        }

    } // namespace sift
} // namespace cqp
