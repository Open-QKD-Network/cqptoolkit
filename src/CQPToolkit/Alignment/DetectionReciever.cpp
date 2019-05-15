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

namespace cqp
{
    namespace align
    {

        // storage unit for the constexpr defined in the header
        constexpr SystemParameters DetectionReciever::DefaultSystemParameters;

        DetectionReciever::DetectionReciever(const SystemParameters &parameters) :
            rng{new RandomNumber()},
            gating{rng, parameters.slotWidth, parameters.pulseWidth},
            drift(parameters.slotWidth, parameters.pulseWidth)
        {

        }

        DetectionReciever::~DetectionReciever()
        {
            Stop(true);
            transmitter.reset();
            while(!receivedData.empty())
            {
                receivedData.pop();
            }
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

        void DetectionReciever::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {
            transmitter = channel;

            while(!receivedData.empty())
            {
                receivedData.pop();
            }
            Start();
        }

        void DetectionReciever::Disconnect()
        {
            Stop(true);
            transmitter.reset();
            while(!receivedData.empty())
            {
                receivedData.pop();
            }
        }

        bool operator ==(remote::Basis::Type left, Basis right)
        {
            using  RemoteType = remote::Basis::Type;
            return (left == RemoteType::Basis_Type_Diagonal && right == Basis::Diagonal) ||
                   (left == RemoteType::Basis_Type_Circular && right == Basis::Circular) ||
                   (left == RemoteType::Basis_Type_Retiliniear && right == Basis::Retiliniear);
        }

        bool DetectionReciever::SiftDetections(Gating::ValidSlots& validSlots,
                                               const google::protobuf::RepeatedField<int>& basis,
                                               QubitList& qubits, int_fast32_t offset)
        {
            using namespace std;
            bool result = false;

            if(validSlots.size() <= qubits.size() && basis.size() >= static_cast<long>(validSlots.size()))
            {
                size_t outputIndex = 0;
                // walk through each valid record, removing the qubit elements which ether aren't valid or the basis dont match
                for(size_t validSlotIndex = 0u; validSlotIndex < validSlots.size(); validSlotIndex++)
                {
                    // find the qubit index which the current valid index relates to once offset is applied
                    const auto adjustedSlot = offset + static_cast<ssize_t>(validSlots[validSlotIndex]);
                    // is the index still valid
                    if(adjustedSlot >= 0 && static_cast<size_t>(adjustedSlot) < qubits.size())
                    {
                        // does our measured basis match the transmitted basis
                        // **Sifting done here**
                        const auto& mappedBasis = static_cast<remote::Basis::Type>(basis[static_cast<int>(adjustedSlot)]);

                        if(mappedBasis == QubitHelper::Base(qubits[static_cast<size_t>(adjustedSlot)]))
                        {
                            // This qubit was:
                            //   * Detected
                            //   * Not considered noise
                            //   * Measured in the correct basis
                            qubits[outputIndex] = qubits[static_cast<size_t>(adjustedSlot)];
                            outputIndex++;
                        } // if basis match
                        else
                        {
                            // the basis dont match, remove the valid slot
                            validSlots.erase(validSlots.begin() + validSlotIndex);
                            // shift the index so we point to the last element, ready for the next iteration
                            --validSlotIndex;
                        } // else basis match
                    } // if index valid
                } // for valid slots

                // through away the bits on the end
                qubits.resize(validSlots.size());
                result = true;
            } // if lengths valid

            return result;
        } // FilterDetections

        void DetectionReciever::DoWork()
        {
            using namespace std;
            while(!ShouldStop())
            {
                std::unique_ptr<ProtocolDetectionReport> report;

                /*lock scope*/
                {
                    unique_lock<mutex> lock(accessMutex);
                    bool dataReady = false;
                    threadConditional.wait(lock, [&]()
                    {
                        dataReady = !receivedData.empty();
                        return dataReady || state != State::Started;
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
                            for(const auto & marker : response.markers())
                            {
                                markers.emplace(marker.first, marker.second);
                            }
                            auto highest = offsetting.HighestValue(markers, validSlots, *results, 0, 1000);

                            if(highest.value > filterMatchMinimum)
                            {
                                SiftDetections(validSlots, response.basis(), *results, highest.offset);

                                // TODO: calculate security parameter

                            }
                            else
                            {
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

    } // namespace align
} // namespace cqp
