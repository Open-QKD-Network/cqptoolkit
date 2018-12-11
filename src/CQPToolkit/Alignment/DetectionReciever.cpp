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
        gating{parameters.slotWidth, parameters.pulseWidth}
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
                bool dataReady = receivedDataCv.wait_for(lock, threadTimeout, [&](){
                    return !receivedData.empty();
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
                // get the markers from Alice
                auto otherSide = remote::IAlignment::NewStub(transmitter);
                grpc::ClientContext ctx;
                remote::FrameId request;
                request.set_id(report->frame);

                remote::QubitByIndex markers;
                LogStatus(otherSide->GetAlignmentMarks(&ctx, request, &markers));

                // convert the markers to local form
                align::Gating::QubitsBySlot markersConverted;
                for(const auto& marker : markers.qubits())
                {
                    markersConverted[marker.first] = static_cast<Qubit>(marker.second);
                }

                // extract the qubits
                std::unique_ptr<QubitList> results{new QubitList()};
                gating.ExtractQubits(start, end, markersConverted, *results);

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
