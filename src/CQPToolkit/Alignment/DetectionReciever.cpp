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
#include "CQPToolkit/Datatypes/Framing.h"

namespace cqp {
namespace align {

    DetectionReciever::DetectionReciever() :
        gatingHandler(rng)
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

    bool DetectionReciever::SetSystemParameters(const SystemParameters &parameters)
    {
        return gatingHandler.SetSystemParameters(parameters.frameWidth, parameters.slotWidth, parameters.pulseWidth);
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

                unique_ptr<QubitList> results = gatingHandler.BuildHistogram(report->detections, report->frame, transmitter);
                const uint64_t qubitsProcessed = results->size();

                // TODO
                //Emit(&IAlignmentCallback::OnAligned, seq++, move(results));
                if(listener && results)
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
