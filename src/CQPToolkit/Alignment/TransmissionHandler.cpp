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
#include "Algorithms/Alignment/Gating.h"

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

    grpc::Status TransmissionHandler::GetAlignmentMarkers(
            grpc::ServerContext *, const remote::MarkersRequest *request, remote::MarkersResponse *response)
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
               dataReady = false;
               receivedDataCv.wait(lock, [&](){
                   auto it = receivedData.find(request->frameid());
                   if(it != receivedData.end())
                   {
                       // look at the data but leave it on the queue
                       emissions = receivedData.find(request->frameid())->second.get();
                       dataReady = true;
                   }
                   return dataReady;
               });

           } /*lock scope*/

           if(dataReady)
           {
               if(emissions && !emissions->emissions.empty())
               {
                   const auto markersToSend = emissions->emissions.size() / markerFractionToSend;
                   while(response->mutable_markers()->size() < markersToSend)
                   {
                       auto index = rng.RandULong() % emissions->emissions.size();
                       response->mutable_markers()->insert({index, remote::BB84::Type(emissions->emissions[index])});
                   }
                   LOGDEBUG("Sent " + std::to_string(markersToSend) + " markers out of " + std::to_string(emissions->emissions.size()) + " emissions.");
                   if(request->sendallbasis())
                   {
                       for(const auto& qubit : emissions->emissions)
                       {
                           switch (QubitHelper::Base(qubit)) {
                               case Basis::Circular:
                                   response->add_basis(remote::Basis_Type::Basis_Type_Circular);
                                   break;
                               case Basis::Diagonal:
                                   response->add_basis(remote::Basis_Type::Basis_Type_Diagonal);
                                   break;
                               case Basis::Retiliniear:
                                   response->add_basis(remote::Basis_Type::Basis_Type_Retiliniear);
                                   break;
                               case Basis::Invalid:
                                   response->add_basis(remote::Basis_Type::Basis_Type_Basis_Invalid);
                                   break;
                           }
                       }
                   }
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
                dataReady = false;
                receivedDataCv.wait(lock, [&](){
                    auto it = receivedData.find(request->frameid());
                    if(it != receivedData.end())
                    {
                        // pull the data off the queue
                        emissions = move(it->second);
                        receivedData.erase(it);
                        dataReady = true;
                    }
                    return dataReady;
                });

            } /*lock scope*/

            if(dataReady)
            {
                if(emissions){
                    if(!align::Gating::FilterDetections(request->slotids().begin(), request->slotids().end(), emissions->emissions))
                    {
                        LOGERROR("Valid transmissions list is invalid");
                    }

                    SendResults(emissions->emissions, request->securityparameter());
                } else {
                    result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Not prepared correctly");
                }
            }
        } while(!dataReady);

        return result;
    }

    bool TransmissionHandler::SetParameters(remote::DeviceConfig& parameters)
    {
        // TODO:
        //markerFractionToSend = fraction;
        return true;
    }

} // namespace align
} // namespace cqp

