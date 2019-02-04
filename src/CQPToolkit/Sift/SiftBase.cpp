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

        void SiftBase::OnAligned(SequenceNumber seq, double securityParameter, std::unique_ptr<QubitList> rawQubits)
        {
            using namespace std;
            LOGDEBUG("Received aligned qubits");

            // Lock scope
            {
                std::lock_guard<std::mutex>  lock(statesMutex);

                if(collectedStates.find(seq) == collectedStates.end())
                {
                    collectedStates[seq] = move(rawQubits);
                } // if
                else
                {
                    LOGERROR("Duplicate alignment sequence ID");
                } // if else
            } // Lock scope

            statesCv.notify_one();
        } //OnAligned

        void SiftBase::PublishStates(QubitsByFrame::iterator start, QubitsByFrame::iterator end, const remote::AnswersByFrame& answers)
        {
            using std::chrono::high_resolution_clock;
            high_resolution_clock::time_point timerStart = high_resolution_clock::now();

            // calculate the number of bits in the current system.
            constexpr uint8_t bitsPerValue = sizeof(DataBlock::value_type) * CHAR_BIT;
            std::unique_ptr<JaggedDataBlock> siftedData(new JaggedDataBlock());

            JaggedDataBlock::value_type value = 0;
            uint_least8_t offset = 0;
            size_t bitsCollected = 0;

            QubitsByFrame::iterator it = start;
            auto answersIt = answers.answers().begin();

            while(!answers.answers().empty() && it != end)
            {

                LOGTRACE(std::to_string(answers.answers().size()) + " : " + std::to_string(it->first));

                auto seqAnswersIt = answers.answers().at(it->first).answers().begin();
                // grow the storage enough to fit the next set of data
                siftedData->reserve(siftedData->size() + it->second->size() / bitsPerValue);
                bitsCollected += it->second->size();
                // For each qubit
                for(const Qubit& qubit : *it->second)
                {
                    // test if this qubit is valid, ie the basis matched when they were compared.
                    if(*seqAnswersIt)
                    {
                        // shift the bit up to the next available slot and or it with the current value
                        value |= static_cast<JaggedDataBlock::value_type>(QubitHelper::BitValue(qubit)) << offset;
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
                    } //if(*seqAnswersIt == true)

                    ++seqAnswersIt;
                } // For each qubit

                ++it;
                ++answersIt;
            } // while(it != end)


            if(offset != 0)
            {
                // There weren't enough bits to completely fill the last word,
                // Add the remaining, the mask will show which bits are valid
                siftedData->push_back(value);
                siftedData->bitsInLastByte = offset;
            } // if(offset != 0)

            if(siftedData->empty())
            {
                LOGWARN("Empty sifted data.");
            }

            const auto bytesProduced = siftedData->size();
            const auto qubitsDisgarded = bitsCollected - siftedData->NumBits();

            if(listener)
            {
                listener->OnSifted(siftedSequence, move(siftedData));
            }

            siftedSequence++;

            stats.publishTime.Update(high_resolution_clock::now() - timerStart);
            stats.bytesProduced.Update(bytesProduced);
            stats.qubitsDisgarded.Update(qubitsDisgarded);
        } // PublishStates

    } // namespace sift
} // namespace cqp
