/*!
* @file
* @brief SiftReciever
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

        Receiver::Receiver()
        {

        }

        grpc::Status Receiver::VerifyBases(grpc::ServerContext*, const remote::BasisBySiftFrame* request, remote::AnswersByFrame* response)
        {
            using std::chrono::high_resolution_clock;
            using namespace std;
            using google::protobuf::RepeatedField;
            using grpc::Status;
            // wait for incomming data
            /*lock scope*/{
                unique_lock<mutex> lock(collectedStatesMutex);
                collectedStatesCv.wait(lock, [&](){
                    return collectedStates.find(request->basis().begin()->first) != collectedStates.end();
                });
            }/*lock scope*/

            high_resolution_clock::time_point timerStart = high_resolution_clock::now();
            Status result = grpc::Status::OK;

            try
            {
                std::unique_lock<std::mutex>  lock(collectedStatesMutex);

                LOGTRACE("First seq number:" + std::to_string(request->basis().begin()->first));
                QubitsByFrame::iterator firstAnswer =
                    collectedStates.find(request->basis().begin()->first);
                QubitsByFrame::iterator mySeqIt = firstAnswer;

                auto theirSeqIt = request->basis().begin();
                // for each sequence id
                while(theirSeqIt != request->basis().end())
                {
                    if(mySeqIt != collectedStates.end())
                    {
                        // shortcuts for readablility
                        const auto& theirBasisList = theirSeqIt->second;
                        RepeatedField<bool>* myAnswers = (*response->mutable_answers())[theirSeqIt->first].mutable_answers();
                        QubitList::const_iterator myQubitIt = mySeqIt->second->begin();
                        LOGTRACE("Their list: " + std::to_string(theirBasisList.basis_size()) +
                                 " My List: " + std::to_string(mySeqIt->second->size()));

                        if(static_cast<unsigned long>(theirBasisList.basis_size()) == mySeqIt->second->size())
                        {
                            //myAnswers.reserve(theirBasisList.basis_size());
                            // for each basis, compare to our basis
                            for(auto element : theirBasisList.basis())
                            {
                                // store the answer
                                myAnswers->Add(Basis(element) == QubitHelper::Base(*myQubitIt));
                                // move to the next element in our list
                                ++myQubitIt;
                            } // for(Qubit::Basis element : theirBasisList)
                        } // if
                        else
                        {
                            LOGERROR("Length missmatch in basis listSift Sequence ID=" + to_string(mySeqIt->first));
                            result = Status(grpc::StatusCode::OUT_OF_RANGE, "Length missmatch in basis list", "Sequence ID=" + to_string(mySeqIt->first));
                            break;
                        } // else
                    } //if(mySeqIt != collectedStates.end())
                    else
                    {
                        LOGERROR("Failed to find state to compare: Sift Sequence ID=" + to_string(theirSeqIt->first));
                        result = Status(grpc::StatusCode::INVALID_ARGUMENT, "Failed to find state to compare", "Sequence ID=" + to_string(theirSeqIt->first));
                        break;
                    } // else
                    ++theirSeqIt;
                    // find the matching record on our side
                    ++mySeqIt;
                } // while(theirSeqIt != basis.end())

                if(result.ok())
                {
                    // publish the results on our side
                    PublishStates(firstAnswer, mySeqIt, *response);
                }
                else
                {
                    LOGERROR("Sifted data not published");

                } // if(result)

            } // try
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            } // catch

            stats.comparisonTime.Update(high_resolution_clock::now() - timerStart);
            return result;
        } // VerifyBases

    } // namespace Sift
} // namespace cqp
