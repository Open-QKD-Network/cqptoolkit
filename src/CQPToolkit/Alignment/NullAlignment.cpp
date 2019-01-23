/*!
* @file
* @brief NullAlignment
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08/01/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "NullAlignment.h"
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp {
    namespace align {

        NullAlignment::NullAlignment()
        {

        }

        void NullAlignment::OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)
        {

            using namespace std;
            // collect incoming data, notify listeners of new data
            LOGTRACE("Receiving photon report");
            {
                lock_guard<mutex> lock(receivedDataMutex);
                std::unique_ptr<QubitList> results{new QubitList()};
                for(const auto& detection : report->detections)
                {
                    results->push_back(detection.value);
                }
                receivedData.push(move(results));
            }
            receivedDataCv.notify_one();
        }

        void NullAlignment::OnEmitterReport(std::unique_ptr<EmitterReport> report)
        {
            using namespace std;
            // collect incoming data, notify listeners of new data
            LOGTRACE("Receiving emitter report");
            {
                lock_guard<mutex> lock(receivedDataMutex);
                std::unique_ptr<QubitList> results{new QubitList()};
                *results = report->emissions;
                receivedData.push(move(results));
            }
            receivedDataCv.notify_one();
        }

        void NullAlignment::DoWork()
        {
            using namespace std;
            while(!ShouldStop())
            {
                std::unique_ptr<QubitList> report;

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

                if(listener && report && !report->empty())
                {
                    listener->OnAligned(seq++, move(report));
                }
            } // while keepGoing
        }
    } // namespace align
} // namespace cqp