/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 31/7/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "benchmark/benchmark.h"
#include "CQPToolkit/Alignment/TransmissionHandler.h"
#include "Algorithms/Logging/ConsoleLogger.h"

namespace cqp
{
    namespace tests
    {
        static RandomNumber randomness;
        static const size_t photonsPerBurst = 100000;

        void GenerateEmitterReports(size_t numReports, std::vector<std::unique_ptr<EmitterReport>>& reports)
        {
            using std::chrono::high_resolution_clock;
            const auto txDelay = high_resolution_clock::now();

            for(auto count = 0u; count < numReports; count++)
            {
                auto report = std::make_unique<EmitterReport>();
                report->epoc = txDelay;
                report->frame = count +1;
                report->period = std::chrono::nanoseconds(100);
                // start the stream
                google::protobuf::Empty response;
                grpc::ClientContext ctx;

                report->emissions = randomness.RandQubitList(photonsPerBurst);
                reports.emplace_back(move(report));
            }
        }

        static void BM_TranssmitterProcessing(benchmark::State& state)
        {
            ConsoleLogger::Enable();
            grpc::ServerContext ctx;
            align::TransmissionHandler handler;

            std::vector<std::unique_ptr<EmitterReport>> reports;
            GenerateEmitterReports(state.max_iterations, reports);

            remote::MarkersRequest markerRequest;
            remote::MarkersResponse markerResponse;

            markerRequest.set_numofmarkers(photonsPerBurst / 3);
            markerRequest.set_sendallbasis(true);

            remote::ValidDetections validDetections;
            google::protobuf::Empty validDetectionsResponse;
            for(auto index = 0u; index < photonsPerBurst; index++)
            {
                if(randomness.RandULong() % 2)
                {
                    validDetections.mutable_slotids()->Add(index);
                }
            }
            size_t frameId = 1;

            for(auto _ : state)
            {

                markerRequest.set_frameid(frameId);
                validDetections.set_frameid(frameId);

                handler.OnEmitterReport(move(reports[frameId - 1]));
                handler.GetAlignmentMarkers(&ctx, &markerRequest, &markerResponse);
                handler.DiscardTransmissions(&ctx, &validDetections, &validDetectionsResponse);
                frameId++;
            }

            state.SetItemsProcessed(photonsPerBurst);
            state.SetLabel("Alignment processing inc storing emittions, GetAlignmentMarkers and DiscardTransmissions");
        }
        BENCHMARK(BM_TranssmitterProcessing);
    }
}
