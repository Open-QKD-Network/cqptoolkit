/*!
* @file
* @brief GatingResponse
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TransmissionHandler.h"

namespace cqp {
namespace align {

    void TransmissionHandler::OnEmitterReport(std::unique_ptr<EmitterReport> report)
    {
        using namespace std;
        // collect incoming data, notify listeners of new data
        LOGTRACE("Receiving emitter report");
        {
            lock_guard<mutex> lock(receivedDataMutex);
            receivedData[report->frame] = move(report);
        }
        receivedDataCv.notify_one();
    }

    grpc::Status TransmissionHandler::GetAlignmentMarks(
            grpc::ServerContext *, const remote::FrameId *request, remote::QubitByIndex *response)
    {
        using namespace std;
       LOGTRACE("Markers requested");
       grpc::Status result;

       bool dataReady = false;
       do
       {
           const EmitterReport* emissions = nullptr;
           /*lock scope*/{
               unique_lock<mutex> lock(receivedDataMutex);
               dataReady = receivedDataCv.wait_for(lock, threadTimeout, [&](){
                   bool result = false;
                   auto it = receivedData.find(request->id());
                   if(it != receivedData.end())
                   {
                       // look at the data but leave it on the queue
                       emissions = receivedData.find(request->id())->second.get();
                       result = true;
                   }
                   return result;
               });

           } /*lock scope*/

           if(dataReady)
           {
               if(emissions && !emissions->emissions.empty())
               {
                   const auto markersToSend = emissions->emissions.size() / markerFractionToSend;
                   while(response->mutable_qubits()->size() < markersToSend)
                   {
                       auto index = rng.RandULong() % emissions->emissions.size();
                       response->mutable_qubits()->insert({index, remote::BB84::Type(emissions->emissions[index])});
                   }
                   LOGDEBUG("Sent " + std::to_string(markersToSend) + " markers out of " + std::to_string(emissions->emissions.size()) + " emissions.");
               } else {
                   result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Not prepared correctly");
               }
           }
       } while(!dataReady);

       return result;
    }

    grpc::Status TransmissionHandler::DiscardTransmissions(grpc::ServerContext *, const remote::ValidDetections *request, google::protobuf::Empty *)
    {
        using namespace std;
        LOGDEBUG("Told to keep " + std::to_string(request->slotids().size()) + " slots");
        grpc::Status result;

        bool dataReady = false;

        do {
            std::unique_ptr<EmitterReport> emissions;
            /*lock scope*/{
                unique_lock<mutex> lock(receivedDataMutex);
                dataReady = receivedDataCv.wait_for(lock, threadTimeout, [&](){
                    bool result = false;
                    auto it = receivedData.find(request->frameid());
                    if(it != receivedData.end())
                    {
                        // pull the data off the queue
                        emissions = move(it->second);
                        receivedData.erase(it);
                        result = true;
                    }
                    return result;
                });

            } /*lock scope*/

            if(dataReady)
            {
                if(emissions){
                    unique_ptr<QubitList> output(new QubitList);

                    output->clear();
                    output->reserve(static_cast<size_t>(request->slotids().size()));

                    for(auto validSlot : request->slotids())
                    {
                        if(validSlot < emissions->emissions.size())
                        {
                            output->push_back(emissions->emissions[validSlot]);
                        } else {
                            LOGERROR("Detected transmissions longer than sent transmissions");
                            result = grpc::Status(grpc::StatusCode::OUT_OF_RANGE, "Slot id greater than last value sent");
                            break; // for
                        }
                    }

                    // TODO pass this off onto another thread
                    if(listener)
                    {
                        listener->OnAligned(seq++, move(output));
                    }
                } else {
                    result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Not prepared correctly");
                }
            }
        } while(!dataReady);

        return result;
    }

} // namespace align
} // namespace cqp
