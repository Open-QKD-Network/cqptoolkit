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
#include "Receiver.h"
#include <climits>
#include "Stats.h"

namespace cqp
{
    namespace sift
    {

        Receiver::Receiver() :
            SiftBase::SiftBase(statesMutex, statesCv)
        {

        }

        grpc::Status Receiver::VerifyBases(grpc::ServerContext*, const remote::BasisBySiftFrame* request, remote::AnswersByFrame* response)
        {
            LOGTRACE("");
            using std::chrono::high_resolution_clock;
            using namespace std;
            using google::protobuf::RepeatedField;
            using grpc::Status;

            Status result = grpc::Status(grpc::StatusCode::ABORTED, "Sift: No data available");
            QubitsByFrame statesToWorkOn;

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
                            statesToWorkOn.emplace(requested.first, move(collectedStates.find(requested.first)->second));
                        }

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
                if(static_cast<uint>(theirList->second.basis_size()) == stateList.second->size())
                {
                    // a lias for the reply list for this frame number
                    RepeatedField<bool>* myAnswers = (*response->mutable_answers())[theirList->first].mutable_answers();
                    myAnswers->Resize(theirList->second.basis().size(), false);

                    // for each basis, compare to our basis, putting the answer in myAnswers
                    std::transform(theirList->second.basis().begin(), theirList->second.basis().end(),
                                   stateList.second->begin(),
                                   myAnswers->begin(),
                                   [](auto left, auto right)
                    {
                        return Basis(left) == QubitHelper::Base(right);
                    });

                } // if sizes match
                else
                {
                    LOGERROR("Length mismatch in basis listSift Sequence ID=" + to_string(stateList.first));
                    result = Status(grpc::StatusCode::OUT_OF_RANGE, "Sift: Length mismatch in basis list", "Sequence ID=" + to_string(stateList.first));
                }

                stats.comparisonTime.Update(high_resolution_clock::now() - timerStart);

            } // for statesToWorkOn

            // publish the results on our side
            PublishStates(statesToWorkOn.begin(), statesToWorkOn.end(), *response);

            return result;
        } // VerifyBases

    } // namespace Sift
} // namespace cqp
