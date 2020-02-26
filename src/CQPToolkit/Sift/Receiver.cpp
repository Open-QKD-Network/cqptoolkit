/*!
* @file
* @brief BB84
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Receiver.h"
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{
    namespace sift
    {

        Receiver::Receiver(unsigned int framesBeforeVerify) :
            WorkerThread (),
            SiftBase (statesMutex, statesCv),
            minFramesBeforeVerify(framesBeforeVerify)
        {
        }

        void Receiver::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {
            {
                //lock scope
                std::lock_guard<std::mutex>  lock(statesMutex);
                collectedStates.clear();
                siftedSequence = 0;
            }// lock scope

            verifier = remote::ISift::NewStub(channel);

            if(verifier == nullptr)
            {
                LOGERROR("Failed to get stub");
            }
            else
            {
                Start();
            }
        }

        void Receiver::Disconnect()
        {
            {
                //lock scope
                std::lock_guard<std::mutex>  lock(statesMutex);
                collectedStates.clear();
                siftedSequence = 0;
            }// lock scope

            Stop(true);
            verifier = nullptr;
        }

        void Receiver::OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)
        {
            using namespace std;
            LOGTRACE("Received aligned qubits");

            // Lock scope
            {
                std::lock_guard<std::mutex>  lock(statesMutex);

                if(collectedStates.find(report->frame) == collectedStates.end())
                {
                    collectedStates[report->frame] = move(report);
                } // if
                else
                {
                    LOGERROR("Duplicate alignment sequence ID");
                } // if else
            } // Lock scope

            statesCv.notify_all();
        }

        Receiver::~Receiver()
        {
            Disconnect();
        }

        void Receiver::DoWork()
        {
            using namespace std;

            SequenceNumber firstSeq = 0;
            bool result = true;

            while(!ShouldStop())
            {
                StatesList statesToWorkOn;
                IntensitiesByFrame intensitiesToWorkOn;
                remote::BasisBySiftFrame basis;
                remote::AnswersByFrame answers;

                /*lock scope*/
                {
                    std::unique_lock<std::mutex>  lock(statesMutex);
                    LOGTRACE("Waiting...");
                    // Wait for data to be available
                    result = statesCv.wait_for(lock, threadTimeout, bind(&Receiver::ValidateIncomming, this, firstSeq));

                    LOGTRACE("Trigggered");
                    if(result && !collectedStates.empty())
                    {
                        auto it = collectedStates.find(firstSeq);

                        do
                        {
                            statesToWorkOn[it->first] = move(it->second);;
                            // remove it from the list
                            collectedStates.erase(it);
                            // look for the next item in the list
                            firstSeq++;
                            it = collectedStates.find(firstSeq);
                        }
                        while(it != collectedStates.end()); // stop when we reach the end or there's a gap in the data

                    } // if result

                }/*lock scope*/
                // unlock to allow more to be added

                // result will be false if it timed out
                if(result)
                {
                    auto it = statesToWorkOn.begin();
                    while(it != statesToWorkOn.end())
                    {
                        // extract the basis from the qubits
                        auto& currentList = (*basis.mutable_basis())[it->first];

                        for(auto& detection : it->second->detections)
                        {
                            // convert to basis value and add to the list
                            currentList.add_basis(remote::Basis::Type((QubitHelper::Base(detection.value))));
                        } // for

                        ++it;
                    } // while(it != end)

                    // send the bases to alice
                    if(verifier)
                    {
                        grpc::ClientContext ctx;
                        result = LogStatus(
                                     verifier->VerifyBases(&ctx, basis, &answers)).ok();

                        if(result)
                        {
                            // Process the results
                            PublishStates(statesToWorkOn.begin(), statesToWorkOn.end(), answers);
                        }

                    } // if
                    else
                    {
                        LOGERROR("Sift: No verifier");
                    }
                } // if(result)
            } // while(!ShouldStop())

            LOGTRACE("Transmitter DoWork Leaving");
        } // DoWork

        bool Receiver::ValidateIncomming(SequenceNumber firstSeq) const
        {
            bool result = false;
            try
            {
                if(minFramesBeforeVerify > 1 && collectedStates.size() > 1)
                {
                    auto it = collectedStates.begin();

                    if(it->first == firstSeq)
                    {
                        SequenceNumber prevSeq = it->first;
                        // skip the first element
                        ++it;
                        SequenceNumber numCollected = 1;
                        while(it != collectedStates.end())
                        {
                            // verify that the sequence is contiguous
                            if(it->first == prevSeq + 1)
                            {
                                prevSeq = it->first;
                                numCollected++;
                                ++it;
                                // check if we have enough
                                if(numCollected >= minFramesBeforeVerify)
                                {
                                    result = true;
                                    break;
                                } // if
                            } // if
                            else
                            {
                                // stop walking through the data, there's a hole
                                break;
                            } // else
                        } // while(it != collectedStates.end())
                    } // if firstSeq ready
                    else
                    {
                        LOGTRACE("Waiting for first seq num: " + std::to_string(firstSeq));
                    }
                } // if
                else if(minFramesBeforeVerify == 1 && !collectedStates.empty())
                {
                    LOGTRACE("FirstSeq=" + std::to_string(firstSeq) + " collected first = " + std::to_string(collectedStates.begin()->first));
                    auto it = collectedStates.begin();
                    do
                    {
                        result = it->first == firstSeq;
                        it++;
                    }
                    while(!result && it != collectedStates.end());
                } // else if

            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
                result = false;
            }
            return result;
        } // ValidateIncomming

        void Receiver::PublishStates(StatesList::const_iterator start, StatesList::const_iterator end,
                                     const remote::AnswersByFrame& answers)
        {
            using std::chrono::high_resolution_clock;
            high_resolution_clock::time_point timerStart = high_resolution_clock::now();

            std::unique_ptr<JaggedDataBlock> siftedData(new JaggedDataBlock());
            JaggedDataBlock::value_type value = 0;
            uint_least8_t offset = 0;

            for(auto listIt = start; listIt != end; listIt++)
            {
                // grow the storage enough to fit the next set of data
                siftedData->reserve(siftedData->size() + listIt->second->detections.size() / bitsPerValue);

                auto answersIt = answers.answers().find(listIt->first);
                if(answersIt != answers.answers().end())
                {
                    for(const auto& qubit : listIt->second->detections)
                    {
                        PackQubit(qubit.value, qubit.time.count(), answersIt->second,
                                  *siftedData, offset, value);
                    }// for qubits
                }
                else
                {
                    LOGERROR("No answers for states.");
                }
            }

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

            double securityParameter = 0.0; // TODO
            // publish the results on our side
            Emit(&ISiftedCallback::OnSifted, siftedSequence, securityParameter, move(siftedData));

            siftedSequence++;

            stats.publishTime.Update(high_resolution_clock::now() - timerStart);
            stats.bytesProduced.Update(bytesProduced);
        }


    } // namespace Sift
} // namespace cqp
