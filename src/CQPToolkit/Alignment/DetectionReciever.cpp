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

namespace cqp {
namespace align {

    // storage unit for the constexpr defined in the header
    constexpr SystemParameters DetectionReciever::DefaultSystemParameters;

    DetectionReciever::DetectionReciever(const SystemParameters &parameters) :
        rng{new RandomNumber()},
        gating{rng, parameters.slotWidth, parameters.pulseWidth}
    {

    }

    void DetectionReciever::OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)
    {
        using namespace std;
        // collect incoming data, notify listeners of new data
        LOGTRACE("Receiving photon report");
        {
            lock_guard<mutex> lock(receivedDataMutex);
            receivedData.push(move(report));
        }
        receivedDataCv.notify_one();
    }

    void DetectionReciever::DoWork()
    {
        using namespace std;
        while(!ShouldStop())
        {
            std::unique_ptr<ProtocolDetectionReport> report;

            /*lock scope*/{
                unique_lock<mutex> lock(receivedDataMutex);
                bool dataReady = false;
                receivedDataCv.wait(lock, [&](){
                    dataReady = !receivedData.empty();
                    return dataReady || ShouldStop();
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
                high_resolution_clock::time_point timerStart = high_resolution_clock::now();

                // isolate the transmission
                DetectionReportList::const_iterator start;
                DetectionReportList::const_iterator end;
                filter.Isolate(report->detections, start, end);

                // extract the qubits
                std::unique_ptr<QubitList> results{new QubitList()};
                Gating::ValidSlots validSlots;
                gating.ExtractQubits(start, end, validSlots, *results);

                {
                    // tell Alice which slots are valid
                    auto otherSide = remote::IAlignment::NewStub(transmitter);
                    grpc::ClientContext ctx;

                    remote::ValidDetections request;
                    google::protobuf::Empty response;

                    // copy the slots over
                    request.mutable_slotids()->Resize(validSlots.size(), 0);
                    std::copy(validSlots.cbegin(), validSlots.cend(), request.mutable_slotids()->begin());
                    request.set_frameid(report->frame);

                    LogStatus(otherSide->DiscardTransmissions(&ctx, request, &response));
                }

                const auto qubitsProcessed = results->size();

                if(listener)
                {
                    listener->OnAligned(seq++, move(results));
                }

                stats.timeTaken.Update(high_resolution_clock::now() - timerStart);
                stats.overhead.Update(0.0L);
                stats.qubitsProcessed.Update(qubitsProcessed);
            }
        } // while keepGoing
    }

} // namespace align
} // namespace cqp
