/*!
* @file
* @brief SiftReceiver
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Verifier.h"
#include <climits>
#include "Stats.h"

namespace cqp
{
    namespace sift
    {

        Verifier::Verifier() :
            SiftBase::SiftBase(statesMutex, statesCv)
        {

        }

        grpc::Status Verifier::VerifyBases(grpc::ServerContext*, const remote::BasisBySiftFrame* request,
                                           remote::AnswersByFrame* response)
        {
            LOGTRACE("");
            using std::chrono::high_resolution_clock;
            using namespace std;
            using google::protobuf::RepeatedField;
            using grpc::Status;

            Status result = grpc::Status(grpc::StatusCode::ABORTED, "Sift: No data available");
            EmitterStateList statesToWorkOn;

            if(!request->basis().empty())
            {
                bool dataReady = false;
                // wait for incoming data
                /*lock scope*/
                {
                    unique_lock<mutex> lock(statesMutex);
                    dataReady = statesCv.wait_for(lock, receiveTimeout, [&]()
                    {
                        bool allFound = true;
                        for(auto requested : request->basis())
                        {
                            if(collectedStates.find(requested.first) == collectedStates.end())
                            {
                                allFound = false;
                                break; // for
                            }
                        }
                        return allFound;
                    });

                    if(dataReady)
                    {
                        for(auto requested : request->basis())
                        {
                            statesToWorkOn.emplace(requested.first,
                                                   move(collectedStates.find(requested.first)->second));

                        } // for requested

                        // erase the items we're working on
                        for(auto requested : request->basis())
                        {
                            collectedStates.erase(requested.first);
                        }
                    } // if dataReady
                }/*lock scope*/
            } // if list !empty

            for(auto& stateList : statesToWorkOn)
            {
                high_resolution_clock::time_point timerStart = high_resolution_clock::now();
                result = grpc::Status::OK;

                auto theirList = request->basis().find(stateList.first);
                // alias for the reply list for this frame number
                auto myBasesAnswers = (*response->mutable_answers())[theirList->first].mutable_answers();

                // for each basis, compare to our basis, putting the answer in myAnswers
                for(const auto& detected : theirList->second.indexedbasis())
                {
                    if(detected.index() < stateList.second->emissions.size())
                    {
                        myBasesAnswers->Add(QubitHelper::Base(stateList.second->emissions[detected.index()]) ==
                                Basis(detected.basis()));
                    } else {
                        LOGERROR("Invalid index: " + std::to_string(detected.index()));
                    }
                }

                if(!discardedIntensities.empty() && !stateList.second->intensities.empty())
                {
                    RepeatedField<uint32_t>* myIntensityAnswers = (*response->mutable_answers())[theirList->first].mutable_intensity();
                    myIntensityAnswers->Resize(stateList.second->intensities.size(),  0);
                    // copy the intensities into the answers
                    copy(stateList.second->intensities.begin(), stateList.second->intensities.end(), myIntensityAnswers->begin());
                }

                stats.comparisonTime.Update(high_resolution_clock::now() - timerStart);

            } // for statesToWorkOn

            PublishStates(statesToWorkOn.begin(), statesToWorkOn.end(), *response);

            return result;
        } // VerifyBases

        void Verifier::PublishStates(EmitterStateList::const_iterator start, EmitterStateList::const_iterator end,
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
                siftedData->reserve(siftedData->size() + listIt->second->emissions.size() / bitsPerValue);

                auto answersIt = answers.answers().find(listIt->first);
                if(answersIt != answers.answers().end())
                {
                    for(auto qubitIndex = 0u; qubitIndex < listIt->second->emissions.size(); qubitIndex++)
                    {
                        PackQubit(listIt->second->emissions[qubitIndex], qubitIndex, answersIt->second,
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

            LOGINFO("Sifted bytes: " + std::to_string(bytesProduced));
            double securityParameter = 0.0; // TODO
            // publish the results on our side
            Emit(&ISiftedCallback::OnSifted, siftedSequence, securityParameter, move(siftedData));

            siftedSequence++;

            stats.publishTime.Update(high_resolution_clock::now() - timerStart);
            stats.bytesProduced.Update(bytesProduced);
        }

        void Verifier::OnEmitterReport(std::unique_ptr<EmitterReport> report)
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

        void Verifier::Connect(std::shared_ptr<grpc::ChannelInterface>)
        {
            std::lock_guard<std::mutex>  lock(statesMutex);
            collectedStates.clear();
            siftedSequence = 0;
        }

        void Verifier::Disconnect()
        {
            std::lock_guard<std::mutex>  lock(statesMutex);
            collectedStates.clear();
            siftedSequence = 0;
        }

    } // namespace Sift
} // namespace cqp
