/*!
* @file
* @brief DetectionReciever
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DetectionReciever.h"
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Alignment/Offsetting.h"

namespace cqp {
namespace align {

    // storage unit for the constexpr defined in the header
    constexpr SystemParameters DetectionReciever::DefaultSystemParameters;

    DetectionReciever::DetectionReciever(const SystemParameters &parameters) :
        rng{new RandomNumber()},
        gating{rng, parameters.slotWidth, parameters.pulseWidth},
        drift(parameters.slotWidth, parameters.pulseWidth)
    {

    }

    void DetectionReciever::OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)
    {
        using namespace std;
        // collect incoming data, notify listeners of new data
        LOGTRACE("Receiving photon report");
        {
            lock_guard<mutex> lock(accessMutex);
            receivedData.push(move(report));
        }
        threadConditional.notify_one();
    }

    void DetectionReciever::DoWork()
    {
        using namespace std;
        while(!ShouldStop())
        {
            std::unique_ptr<ProtocolDetectionReport> report;

            /*lock scope*/{
                unique_lock<mutex> lock(accessMutex);
                bool dataReady = false;
                threadConditional.wait(lock, [&](){
                    dataReady = !receivedData.empty();
                    return dataReady || state == State::Stop;
                });

                if(dataReady)
                {
                    report = move(receivedData.front());
                    receivedData.pop();
                }
            } /*lock scope*/

            if(report && !report->detections.empty())
            {
                using namespace std::chrono;
                using std::chrono::high_resolution_clock;
                grpc::Status result;
                high_resolution_clock::time_point timerStart = high_resolution_clock::now();

                // isolate the transmission
                DetectionReportList::const_iterator start;
                DetectionReportList::const_iterator end;
                filter.Isolate(report->detections, start, end);

                // extract the qubits
                std::unique_ptr<QubitList> results{new QubitList()};
                Gating::ValidSlots validSlots;
                gating.SetDrift(drift.Calculate(start, end));
                gating.ExtractQubits(start, end, validSlots, *results);

                auto otherSide = remote::IAlignment::NewStub(transmitter);

                double securityParameter = 0.0;

                {
                    grpc::ClientContext ctx;

                    remote::MarkersRequest request;
                    remote::MarkersResponse response;

                    request.set_frameid(report->frame);
                    request.set_sendallbasis(true);
                    // this seems like it's not going to cope under different situations
                    request.set_numofmarkers(validSlots.size() * 0.1);
                    result = LogStatus(otherSide->GetAlignmentMarkers(&ctx, request, &response));

                    if(result.ok())
                    {
                        align::Offsetting offsetting(0);
                        QubitsBySlot markers;
                        markers.reserve(response.markers().size());
                        for(auto marker : response.markers())
                        {
                            markers.emplace(marker.first, marker.second);
                        }
                        auto highest = offsetting.HighestValue(markers, validSlots, *results, 0, 1000);

                        if(highest.value > 0.8)
                        {
                            Gating::FilterDetections(validSlots.cbegin(), validSlots.cend(), *results, highest.offset);

                            // TODO: calculate security parameter

                        } else {
                            LOGERROR("Match of " + to_string(highest.value) + " is too low to generate key");
                            result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Can't find match");
                        }
                    }
                }

                if(result.ok())
                {
                    // tell Alice which slots are valid

                    grpc::ClientContext ctx;
                    remote::ValidDetections request;
                    google::protobuf::Empty response;

                    // copy the slots over
                    request.mutable_slotids()->Resize(validSlots.size(), 0);
                    std::copy(validSlots.cbegin(), validSlots.cend(), request.mutable_slotids()->begin());
                    request.set_frameid(report->frame);


                    LogStatus(otherSide->DiscardTransmissions(&ctx, request, &response));
                }

                if(result.ok())
                {
                    const auto qubitsProcessed = results->size();

                    SendResults(*results, securityParameter);

                    stats.timeTaken.Update(high_resolution_clock::now() - timerStart);
                    stats.overhead.Update(0.0L);
                    stats.qubitsProcessed.Update(qubitsProcessed);
                }
            }
        } // while keepGoing
    }

    bool DetectionReciever::SetParameters(config::DeviceConfig& parameters)
    {
        // TODO
        return true;
    }

} // namespace align
} // namespace cqp
