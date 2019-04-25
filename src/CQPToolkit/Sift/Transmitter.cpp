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
#include "Transmitter.h"
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{
    namespace sift
    {

        Transmitter::Transmitter(unsigned int framesBeforeVerify) :
            WorkerThread (),
            SiftBase (statesMutex, statesCv),
            minFramesBeforeVerify(framesBeforeVerify)
        {
        }

        void Transmitter::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {
            SiftBase::Connect(channel);

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

        void Transmitter::Disconnect()
        {
            Stop(true);
            verifier = nullptr;
            SiftBase::Disconnect();
        }

        Transmitter::~Transmitter()
        {
            Disconnect();
        }

        void Transmitter::DoWork()
        {
            using namespace std;

            SequenceNumber firstSeq = 0;
            bool result = true;

            while(!ShouldStop())
            {
                QubitsByFrame statesToWorkOn;
                remote::BasisBySiftFrame basis;
                remote::AnswersByFrame answers;

                /*lock scope*/
                {
                    std::unique_lock<std::mutex>  lock(statesMutex);
                    LOGTRACE("Waiting...");
                    // Wait for data to be available
                    result = statesCv.wait_for(lock, threadTimeout, bind(&Transmitter::ValidateIncomming, this, firstSeq));

                    LOGTRACE("Trigggered");
                    if(result && !collectedStates.empty())
                    {
                        QubitsByFrame::iterator it = collectedStates.find(firstSeq);

                        do
                        {
                            statesToWorkOn[it->first] = move(it->second);
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
                    QubitsByFrame::iterator it = statesToWorkOn.begin();
                    while(it != statesToWorkOn.end())
                    {
                        // extract the basis from the qubits
                        auto& currentList = (*basis.mutable_basis())[it->first];

                        for(auto& qubit : *it->second)
                        {
                            // convert to basis value and add to the list
                            currentList.add_basis(remote::Basis::Type((QubitHelper::Base(qubit))));
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

        bool Transmitter::ValidateIncomming(SequenceNumber firstSeq) const
        {
            bool result = false;
            try
            {
                if(minFramesBeforeVerify > 1 && collectedStates.size() > 1)
                {
                    QubitsByFrame::const_iterator it = collectedStates.begin();

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


    } // namespace Sift
} // namespace cqp
