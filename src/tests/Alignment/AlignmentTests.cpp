/*!
* @file
* @brief Alignment
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 5/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "AlignmentTests.h"
#include "CQPToolkit/Alignment/Alignment.h"
#include <ctime>
#include "CQPToolkit/Util/ConsoleLogger.h"
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "CQPToolkit/Alignment/DetectionGating.h"
#include "CQPToolkit/Simulation/DummyTransmitter.h"
#include "CQPToolkit/Simulation/DummyTimeTagger.h"
#include "CQPToolkit/Alignment/TransmissionHandler.h"
#include "CQPToolkit/Alignment/DetectionReciever.h"
#include <chrono>
#include "CQPToolkit/Util/DataFile.h"

namespace cqp
{
    namespace test
    {

        using ::testing::_;
        using namespace ::testing;

        AlignmentTests::AlignmentTests()
        {
            using namespace std;
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Debug);

            grpc::ServerBuilder builder;
            int listenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
            builder.RegisterService(&txCallback);

            server= builder.BuildAndStart();

            clientChannel = grpc::CreateChannel("localhost:" + std::to_string(listenPort), grpc::InsecureChannelCredentials());

        }

        TEST_F(AlignmentTests, Gating)
        {
            AlignmentTestData testData;

            testData.emissions = rng.RandQubitList(100);
            PicoSeconds time{1};
            for(auto qubit : testData.emissions)
            {
                if(rng.SRandInt() % 2)
                {
                    // randomly detect the correct value
                    DetectionReport report;
                    report.time = time;
                    report.value = qubit;
                //} else {
                //    report.value = rng.RandQubit();
                //}
                    testData.detections.push_back(report);
                }
                time+= PicoSeconds(10001);
            }

            LOGDEBUG("There are " + std::to_string(testData.emissions.size()) + " emissions and " + std::to_string(testData.detections.size()) + " detections.");

            const PicoSeconds pulseWidth   {100};
            const PicoSeconds slotWidth  {10000};
            const PicoSeconds frameWidth{testData.emissions.size() * slotWidth.count()};

            align::DetectionGating gating(rng);
            gating.SetSystemParameters(frameWidth, slotWidth, pulseWidth, 5, 0.01);
            gating.SetNumberThreads(1);
            gating.ResetDrift(align::DetectionGating::PicoSecondOffset(-100000));
            QubitList fixedEmissions;
            std::set<uint64_t> markerIds;

            EXPECT_CALL(txCallback, GetAlignmentMarks(_, _, _)).WillRepeatedly(
                        Invoke([&](grpc::ServerContext *, const remote::FrameId* request, remote::QubitByIndex *response) -> grpc::Status{
                LOGINFO("Markers requested");

               while(response->mutable_qubits()->size() < (testData.detections.size() / 2))
               {
                    auto index = rng.RandULong() % testData.emissions.size();
                    markerIds.insert(index);
                    (*response->mutable_qubits())[index] = remote::BB84::Type(testData.emissions[index]);
               }
                            LOGDEBUG("Markers: " + std::to_string(markerIds.size()));
                return grpc::Status();
            }));

            EXPECT_CALL(txCallback, DiscardTransmissions(_, _, _)).WillRepeatedly(
                        Invoke([&](grpc::ServerContext *, const remote::ValidDetections *request, google::protobuf::Empty *) -> grpc::Status{
                LOGINFO("Told to keep " + std::to_string(request->slotids().size()) + " slots");

                            for(auto validSlot : request->slotids())
                            {
                                // drop markers
                                if(markerIds.find(validSlot) == markerIds.end())
                                {
                                    if(validSlot < testData.emissions.size())
                                    {
                                        fixedEmissions.push_back(testData.emissions[validSlot]);
                                    }
                                }
                            }
                            return grpc::Status();
            }));
            
            const auto startTime = std::chrono::high_resolution_clock::now();
            auto results = gating.BuildHistogram(testData.detections, 1, clientChannel);
            const auto timeTaken = std::chrono::high_resolution_clock::now() - startTime;
            std::stringstream timeString;
            timeString << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeTaken).count() << "ms";
            LOGDEBUG(timeString.str());
            ASSERT_GT(results->size(), 0u);
            ASSERT_EQ(results->size(), fixedEmissions.size());
            ASSERT_EQ(*results, fixedEmissions);
        }

        TEST_F(AlignmentTests, SimlatedSource)
        {
            RandomNumber rng;
            sim::DummyTimeTagger timeTagger(&rng);
            sim::DummyTransmitter photons(&rng);
            align::DetectionReciever detection;
            align::TransmissionHandler txHandler;
            MockAlignmentCallback txCallback;
            MockAlignmentCallback rxCallback;

            // setup classes
            grpc::ServerBuilder builder;
            int listenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
            builder.RegisterService(&txHandler);
            builder.RegisterService(static_cast<remote::IPhotonSim::Service*>(&timeTagger));

            auto server= builder.BuildAndStart();

            auto clientChannel = grpc::CreateChannel("localhost:" + std::to_string(listenPort), grpc::InsecureChannelCredentials());

            timeTagger.Attach(&detection);
            photons.Attach(&txHandler);

            detection.Attach(&rxCallback);
            txHandler.Attach(&txCallback);

            std::condition_variable waitCv;
            std::mutex waitMutex;
            std::atomic<uint> alignedCalled {0};
            QubitList txIncomming;
            SequenceNumber txSeq = 0;
            QubitList rxIncomming;
            SequenceNumber rxSeq = 0;

            EXPECT_CALL(txCallback, OnAligned2(_, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, QubitList* rawQubits){
                txSeq = seq;
                txIncomming.insert(txIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            EXPECT_CALL(rxCallback, OnAligned2(_, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, QubitList* rawQubits){
                rxSeq = seq;
                rxIncomming.insert(rxIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            photons.Connect(clientChannel);
            detection.Connect(clientChannel);

            google::protobuf::Timestamp timeStamp;
            timeStamp.set_seconds(std::chrono::high_resolution_clock::now().time_since_epoch().count());

            const int itterations = 10;
            for(int i = 0; i < itterations; i++)
            {
                photons.StartFrame();
                ASSERT_TRUE(timeTagger.StartDetecting(nullptr, &timeStamp, nullptr).ok());
                photons.Fire();
                timeStamp.set_seconds(std::chrono::high_resolution_clock::now().time_since_epoch().count());
                ASSERT_TRUE(timeTagger.StopDetecting(nullptr, &timeStamp, nullptr).ok());
                photons.EndFrame();
            }

            std::unique_lock<std::mutex> lock(waitMutex);
            bool dataArrived = waitCv.wait_for(lock, std::chrono::seconds(60), [&](){
                return alignedCalled == itterations * 2;
            });
            detection.Disconnect();

            ASSERT_TRUE(dataArrived);
            ASSERT_GT(txSeq, 0) << "Invalid sequence number";
            ASSERT_EQ(txSeq, rxSeq) << "Sequence numbers don't match";
            ASSERT_GT(txIncomming.size(), 0) << "No data";
            ASSERT_EQ(txIncomming, rxIncomming) << "Data doesn't match";
        }

        TEST_F(AlignmentTests, RealData)
        {
            using namespace std::chrono;
            RandomNumber rng;
            std::unique_ptr<EmitterReport> emissions { new EmitterReport() };
            std::unique_ptr<ProtocolDetectionReport> detectionReport { new ProtocolDetectionReport() };
            fs::DataFile::ReadNOXDetections("BobDetections.bin", detectionReport->detections);
            fs::DataFile::ReadPackedQubits("AliceRandom.bin", emissions->emissions);

            SystemParameters params;
            params.slotWidth = nanoseconds(100);
            params.pulseWidth = nanoseconds(1);
            params.frameWidth = emissions->emissions.size() * params.slotWidth;

            emissions->epoc = high_resolution_clock::now();
            detectionReport->epoc = emissions->epoc;

            emissions->frame = 0;
            detectionReport->frame = emissions->frame;

            emissions->period = params.slotWidth;

            align::DetectionReciever detection;
            align::TransmissionHandler txHandler;

            // setup classes
            grpc::ServerBuilder builder;
            int listenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
            builder.RegisterService(&txHandler);

            auto server= builder.BuildAndStart();

            auto clientChannel = grpc::CreateChannel("localhost:" + std::to_string(listenPort), grpc::InsecureChannelCredentials());

            MockAlignmentCallback txCallback;
            MockAlignmentCallback rxCallback;
            detection.Attach(&rxCallback);
            txHandler.Attach(&txCallback);
            detection.SetSystemParameters(params);
            detection.Connect(clientChannel);


            std::condition_variable waitCv;
            std::mutex waitMutex;
            std::atomic<uint> alignedCalled {0};
            QubitList txIncomming;
            SequenceNumber txSeq = 0;
            QubitList rxIncomming;
            SequenceNumber rxSeq = 0;

            EXPECT_CALL(txCallback, OnAligned2(_, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, QubitList* rawQubits){
                txSeq = seq;
                txIncomming.insert(txIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            EXPECT_CALL(rxCallback, OnAligned2(_, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, QubitList* rawQubits){
                rxSeq = seq;
                rxIncomming.insert(rxIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            txHandler.OnEmitterReport(move(emissions));
            detection.OnPhotonReport(move(detectionReport));

            std::unique_lock<std::mutex> lock(waitMutex);
            bool dataArrived = waitCv.wait_for(lock, std::chrono::seconds(60), [&](){
                return alignedCalled == 2;
            });
            detection.Disconnect();

            ASSERT_TRUE(dataArrived);
            ASSERT_EQ(txSeq, rxSeq) << "Sequence numbers don't match";
            ASSERT_GT(txIncomming.size(), 0) << "No data";
            ASSERT_EQ(txIncomming.size(), rxIncomming.size()) << "Data sizes don't match";

            fs::DataFile::WriteQubits(txIncomming, "AlignedBytesTx.bin");
            fs::DataFile::WriteQubits(rxIncomming, "AlignedBytesRx.bin");
            // TODO
        }
    } // namespace test
} // namespace cqp
