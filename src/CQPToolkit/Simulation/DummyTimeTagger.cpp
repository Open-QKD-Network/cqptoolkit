/*!
* @file
* @brief DummyTimeTagger
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DummyTimeTagger.h"
#include <chrono>
#include <mutex>
#include "CQPToolkit/Interfaces/IRandom.h"
#include "CQPToolkit/Statistics/Stat.h"

namespace cqp
{

    namespace sim
    {
        DummyTimeTagger::DummyTimeTagger(IRandom* randomSource) :
            rng(randomSource)
        {

        }

        grpc::Status DummyTimeTagger::OnPhoton(grpc::ServerContext *, const remote::FakeDetection* request, google::protobuf::Empty *)
        {
            using namespace std::chrono;
            grpc::Status result(grpc::Status::OK);
            DetectionReportList detections;
            detections.reserve(request->values().qubits().size());
            uint count = 0;

            for(auto qubit : request->values().qubits())
            {
                DetectionReport report;
                report.time = PicoSeconds(request->periodpicoseconds()) * count;
                report.value = static_cast<uint8_t>(qubit);
                detections.push_back(report);
                count++;
            }

            // TODO: Randomly pick bases and simulate errors
            LOGTRACE("Received " + std::to_string(detections.size()) + " photons");

            std::lock_guard<std::mutex> lock(collectedPhotonsMutex);
            collectedPhotons.assign(detections.begin(), detections.end());
            return result;
        } // OnPhoton

        grpc::Status DummyTimeTagger::StartDetecting(grpc::ServerContext*, const google::protobuf::Timestamp*, google::protobuf::Empty*)
        {
            grpc::Status result;
            using std::chrono::high_resolution_clock;
            std::lock_guard<std::mutex> lock(collectedPhotonsMutex);

            epoc = high_resolution_clock::now();
            return result;
        } //StartDetecting

        grpc::Status DummyTimeTagger::StopDetecting(grpc::ServerContext*, const google::protobuf::Timestamp*, google::protobuf::Empty*)
        {
            grpc::Status result;
            using std::chrono::high_resolution_clock;
            std::unique_ptr<ProtocolDetectionReport> report(new ProtocolDetectionReport);

            {
                // lock scope
                std::lock_guard<std::mutex> lock(collectedPhotonsMutex);
                report->detections = collectedPhotons;
                report->epoc = epoc;
                report->frame = frame;
                collectedPhotons.clear();
            } // lock scope

            stats.qubitsReceived.Update(report->detections.size());
            if(listener)
            {
                listener->OnPhotonReport(move(report));
            }
            stats.frameTime.Update(high_resolution_clock::now() - epoc);

            frame++;
            return result;
        } // StopDetecting
    } // namespace sim
} // namespace cqp
