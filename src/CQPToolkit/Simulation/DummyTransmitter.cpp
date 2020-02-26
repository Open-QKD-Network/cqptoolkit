/*!
* @file
* @brief DummyTransmitter
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DummyTransmitter.h"

#include <chrono>
#include <thread>
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{
    namespace sim
    {
        DummyTransmitter::DummyTransmitter(IRandom* randomSource,
                                           PicoSeconds transmissionDelay,
                                           size_t photonsPerBurst,
                                           Intensity intensityLevels) :
            txDelay(transmissionDelay), randomness(randomSource), photonsPerBurst(photonsPerBurst),
            intensityLevels(intensityLevels)
        {
        }

        DummyTransmitter::~DummyTransmitter()
        {
            Disconnect();
        }

        void DummyTransmitter::Connect(std::shared_ptr<grpc::ChannelInterface> channel)
        {
            detector = remote::IPhotonSim::NewStub(channel);
            frame = 0;
        }

        void DummyTransmitter::Disconnect()
        {
            detector = nullptr;
            frame = 0;
        }

        bool DummyTransmitter::Fire()
        {
            using std::chrono::high_resolution_clock;
            high_resolution_clock::time_point timerStart = high_resolution_clock::now();

            std::unique_ptr<EmitterReport> report(new EmitterReport);
            report->epoc = epoc;
            report->frame = frame;
            report->period = txDelay;
            // start the stream
            google::protobuf::Empty response;
            remote::FakeDetection request;
            grpc::ClientContext ctx;

            request.set_periodpicoseconds(txDelay.count());

            report->emissions = randomness->RandQubitList(photonsPerBurst);
            for(auto qubit : report->emissions)
            {
                request.mutable_values()->add_qubits(static_cast<remote::BB84::Type>(qubit));
            }
            if(intensityLevels > 1)
            {
                // generate intensity valuse
                report->intensities.resize(report->emissions.size());
                for(size_t index = 0u; index < report->emissions.size(); index++)
                {
                    report->intensities[index] = randomness->RandULong() % intensityLevels;
                }
            }

            grpc::Status status = LogStatus(
                                      detector->OnPhoton(&ctx, request, &response));
            std::this_thread::sleep_for(txDelay * request.values().qubits().size());

            // tell the listeners what we sent.
            const auto qubitsTransmitted = report->emissions.size();

            Emit(&IEmitterEventCallback::OnEmitterReport, move(report));

            stats.timeTaken.Update(high_resolution_clock::now() - timerStart);
            stats.qubitsTransmitted.Update(qubitsTransmitted);

            return status.ok();
        }

        void DummyTransmitter::StartFrame()
        {
            using std::chrono::high_resolution_clock;

            epoc = high_resolution_clock::now();
        }

        void DummyTransmitter::EndFrame()
        {
            using std::chrono::high_resolution_clock;
            stats.frameTime.Update(high_resolution_clock::now() - epoc);
            frame++;
        }
    } // namespace sim
} // namespace cqp

