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
            minFramesBeforeVerify(framesBeforeVerify)
        {
        }

        void Transmitter::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {
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
                    std::unique_lock<std::mutex>  lock(collectedStatesMutex);
                    // Wait for data to be available
                    result = collectedStatesCv.wait_for(lock, threadTimeout, bind(&Transmitter::ValidateIncomming, this, firstSeq));

                    if(result)
                    {
                        QubitsByFrame::iterator it = collectedStates.begin();
                        QubitsByFrame::iterator end;

                        SequenceNumber prevSeq = it->first;
                        // skip the first element
                        ++it;
                        while(it != collectedStates.end())
                        {
                            // verify that the sequence is contiguous
                            if(it->first == prevSeq + 1)
                            {
                                prevSeq = it->first;
                                ++it;
                                // don't stop, we may have more to use
                            }
                            else
                            {
                                // stop walking through the data, there's a hole
                                break;
                            }
                        }
                        // it now point to one past the last valid element.
                        end = it;

                        for(auto moveItem = collectedStates.begin(); moveItem != end; moveItem++)
                        {
                            statesToWorkOn[moveItem->first] = move(moveItem->second);
                        }
                        collectedStates.erase(collectedStates.begin(), end);
                    } // if result

                }/*lock scope*/
                // unlock to allow more to be added

                // result will be false if it timed out
                if(result)
                {
                    QubitsByFrame::iterator it = statesToWorkOn.begin();
                    while(it != statesToWorkOn.end())
                    {

                        //(*basis.mutable_basis())[it->first].reserve(it->second.size());
                        // extract the basis from the qubits
                        auto& currentList = (*basis.mutable_basis())[it->first];

                        for(auto& qubit : *it->second)
                        {
                            // convert to basis value and add to the list
                            currentList.add_basis(remote::Basis::Type((QubitHelper::Base(qubit))));
                        } // for

                        // store the start of the next set of elements to process
                        firstSeq = it->first + 1;
                        ++it;
                    } // while(it != end)

                } // if(result)

                // send the bases to alice
                if(result && verifier)
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

            } // while(!ShouldStop())

        } // DoWork

        bool Transmitter::ValidateIncomming(SequenceNumber firstSeq) const
        {
            bool result = false;
            if(minFramesBeforeVerify > 1 && collectedStates.size() > 1 && collectedStates.begin()->first == firstSeq)
            {
                QubitsByFrame::const_iterator it = collectedStates.begin();

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
            } // if
            else if(minFramesBeforeVerify == 1)
            {
                result = collectedStates.size() >= minFramesBeforeVerify && collectedStates.begin()->first == firstSeq;
            } // else if

            return result;
        } // ValidateIncomming


    } // namespace Sift
} // namespace cqp
