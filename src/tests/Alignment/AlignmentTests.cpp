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
#include "Algorithms/Logging/ConsoleLogger.h"
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "Algorithms/Alignment/Gating.h"
#include "Algorithms/Alignment/Drift.h"
#include "CQPToolkit/Simulation/DummyTransmitter.h"
#include "CQPToolkit/Simulation/DummyTimeTagger.h"
#include "CQPToolkit/Alignment/TransmissionHandler.h"
#include "CQPToolkit/Alignment/DetectionReciever.h"
#include <chrono>
#include "Algorithms/Util/DataFile.h"
#include "Algorithms/Alignment/Filter.h"

namespace cqp
{
    namespace tests
    {

        using ::testing::_;
        using namespace ::testing;

        class AlignmentTestData
        {
        public:
            QubitList emissions;
            PicoSeconds emissionPeriod {100000};
            PicoSeconds emissionDelay  {1000};
            DetectionReportList detections;
        };

        AlignmentTests::AlignmentTests() :
            rng{new RandomNumber()}
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

        TEST_F(AlignmentTests, EdgeDetect)
        {
            const std::vector<uint64_t> graph
            {
                6, 6, 6, 45, 46, 56, 90, 89, 43, 5, 7
            };

            const uint64_t cutoff = 43;
            std::vector<uint64_t>::const_iterator edge;
            const auto start = std::chrono::high_resolution_clock::now();
            ASSERT_TRUE(align::Filter::FindThreshold(graph.begin(), graph.end(), cutoff, edge));
            const auto timeTaken = std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::high_resolution_clock::now() - start).count();
            LOGINFO("Edge detection took:" + std::to_string(timeTaken) + "uS");
            ASSERT_EQ(*edge, 5);
            ASSERT_TRUE(align::Filter::FindThreshold<uint64_t>(graph.begin(), graph.end(), cutoff, edge, std::greater<uint64_t>()));
            ASSERT_EQ(*edge, 45);
        }

        TEST_F(AlignmentTests, Filter)
        {
            const std::vector<double> sourceData =
            {
                7.29417067307692E-09,
                1.48550180288462E-09,
                6.48662860576923E-09,
                7.10637019230769E-09,
                1.15891526442308E-05,
                2.80892916165865E-05,
                7.42962439903846E-05,
                8.23380821814904E-05,
                0.000119654039588,
                0.00012715859375,
                0.000130728703425,
                0.000134304456505,
                0.000148888174204,
                0.00015784136869,
                0.000186245673077,
                0.000195275478891,
                0.000197325246019,
                0.000200454708158,
                0.000266329214243,
                0.000282517951848,
                0.000319506788987,
                0.000330188613657,
                0.000338148822491,
                0.000359496886268,
                0.000369971875,
                0.000376218997897,
                0.00038616262207,
                0.000397462663386,
                0.000432693391301,
                0.000446715040941,
                0.000455464152644,
                0.000471892424129,
                0.000489657404973,
                0.000506103145658,
                0.000529094996995,
                0.000535599586839,
                0.000558044886193,
                0.000596782491361,
                0.000617169191331,
                0.000655780068735,
                0.000701755418044,
                0.000713374994366,
                0.000763135866136,
                0.000787848506986,
                0.000848460186298,
                0.000885351308969,
                0.000951124160532,
                0.000974783479192,
                0.000995722164213,
                0.001011112004207,
                0.001011343845778,
                0.001018374384014,
                0.00102048050631,
                0.001044369195087,
                0.00105376468412,
                0.001065795141602,
                0.001075956234976,
                0.001111231265024,
                0.001123138720703,
                0.001127617653245,
                0.001132050610352,
                0.001136911262395,
                0.001137034592849,
                0.00117146345966,
                0.001181935064228,
                0.001201085028546,
                0.001219803573843,
                0.001240518231671,
                0.0012592576247,
                0.001267977304312,
                0.001281692193134,
                0.00131752194073,
                0.001335754146635,
                0.001356880429312,
                0.001361897635592,
                0.001407903307166,
                0.001422088241812,
                0.001428225347431,
                0.001441965686974,
                0.001452444655198,
                0.001465993391301,
                0.001508328526893,
                0.001578866466346,
                0.001589559724309,
                0.001594068567834,
                0.001624364120718,
                0.001635423114483,
                0.001651023114483,
                0.001653395641151,
                0.001658863767653,
                0.00168280485652,
                0.00169300563777,
                0.001757468620418,
                0.001803233383413,
                0.001811952732497,
                0.001820218866436,
                0.001848896844952,
                0.001872514971454,
                0.001898826701472,
                0.001908907354267
            };
            const auto windowOdd = align::Filter::GaussianWindow1D(5.0, 3);
            ASSERT_DOUBLE_EQ(windowOdd[1], 1.0);
            ASSERT_DOUBLE_EQ(windowOdd[0], 0.9801986733067553);
            ASSERT_DOUBLE_EQ(windowOdd[2], windowOdd[0]);

            const auto windowEven = align::Filter::GaussianWindow1D(5.0, 20);
            ASSERT_DOUBLE_EQ(windowEven[0], 0.1644744565771549);
            ASSERT_DOUBLE_EQ(windowEven[9], 0.9950124791926823);
            ASSERT_DOUBLE_EQ(windowEven[9], windowEven[10]);
            ASSERT_DOUBLE_EQ(windowEven[0], windowEven[19]);

            const auto filter = align::Filter::GaussianWindow1D(5.0, 21);
            std::vector<double> convolved;
            const auto start = std::chrono::high_resolution_clock::now();
            align::Filter::ConvolveValid(sourceData.begin(), sourceData.end(),
                                         filter.begin(), filter.end(), convolved);

            const auto timeTaken = std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::high_resolution_clock::now() - start).count();
            LOGINFO("ConvolveValid took:" + std::to_string(timeTaken) + "uS");

            const std::vector<double> expected =
            {
                0.0015103228462488, 0.0016953374641034, 0.001884410855395,
                0.002079474233248, 0.0022799895289948, 0.0024844928285461,
                0.0026927444339927, 0.0029014820373358, 0.003116759496634,
                0.0033318576385059, 0.003547416318038, 0.0037621547727005,
                0.0039744012207246, 0.0041819862805166, 0.0043863338257467,
                0.0045842571432572, 0.0047805413997659, 0.0049790932797432,
                0.005179354655543, 0.005379483159535, 0.0055892653265529,
                0.0058048548169476, 0.0060351494880926, 0.0062792183498177,
                0.0065414601843983, 0.0068226615222205, 0.0071282297420276,
                0.0074544730607389, 0.0078007085562214, 0.0081628570423219,
                0.0085389848202691, 0.0089257379625461, 0.0093161054622381,
                0.0097064550867032, 0.0100893493892537, 0.0104583475913645,
                0.0108094636481657, 0.011139915980673, 0.0114440388340263,
                0.0117225034028596, 0.0119740728355337, 0.0122003791538419,
                0.0124069374785167, 0.0125967661344274, 0.0127740055016062,
                0.0129392978281515, 0.0130986583359229, 0.0132522384569895,
                0.0134067086699054, 0.0135621273665361, 0.0137201616354137,
                0.0138856302874063, 0.0140568101888096, 0.0142352510706916,
                0.014417392819558, 0.0146101547207402, 0.0148104110949041,
                0.0150169233028455, 0.0152272573727848, 0.0154426909538229,
                0.015662455646208, 0.0158883747837894, 0.0161239844730025,
                0.0163637627839681, 0.0166032718106876, 0.0168485144287845,
                0.0170968958929513, 0.0173491232041389, 0.0176031491703294,
                0.0178582647641647, 0.0181158986444529, 0.0183719395923696,
                0.0186291958331789, 0.0188881489335197, 0.019144144177792,
                0.0193980467648773, 0.0196483916014781, 0.0198998625471158,
                0.0201548266119389, 0.0204117137245785
            };
            ASSERT_THAT(convolved, Pointwise(FloatEq(), expected));
        }

        TEST_F(AlignmentTests, Gating)
        {
            const PicoSeconds pulseWidth            {100};
            const std::chrono::nanoseconds slotWidth  {10};
            AlignmentTestData testData;

            testData.emissions = rng->RandQubitList(100000);
            PicoSeconds time{1};
            for(auto qubit : testData.emissions)
            {
                if(rng->SRandInt() % 2)
                {
                    // randomly detect the correct value

                    //} else {
                    //    report.value = rng.RandQubit();
                    //}
                    testData.detections.push_back({time, qubit});
                }
                time+= slotWidth + PicoSeconds(1);
            }

            LOGDEBUG("There are " + std::to_string(testData.emissions.size()) + " emissions and " + std::to_string(testData.detections.size()) + " detections.");

            align::Gating gating(rng, slotWidth, pulseWidth);
            align::Drift drift(slotWidth, pulseWidth, slotWidth * 100);
            QubitList alignedDetections;
            align::Gating::ValidSlots validSlots;

            const auto startTime = std::chrono::high_resolution_clock::now();
            const double calculatedDrift = drift.Calculate(testData.detections.cbegin(), testData.detections.cend());
            // TODO: Get drift calculation closer
            ASSERT_NEAR(calculatedDrift, 10.0e-5, 0.005e-5);
            gating.SetDrift(calculatedDrift);

            gating.ExtractQubits(testData.detections.cbegin(), testData.detections.cend(), validSlots, alignedDetections);

            const auto timeTaken = std::chrono::high_resolution_clock::now() - startTime;
            std::stringstream timeString;
            timeString << "Time taken: " << std::chrono::duration_cast<std::chrono::microseconds>(timeTaken).count() << "us";

            // do Alice's bit
            ASSERT_TRUE(align::Gating::FilterDetections(validSlots.cbegin(), validSlots.cend(), testData.emissions));

            LOGDEBUG(timeString.str());
            // have the emissions been shunk to match the detection list
            ASSERT_EQ(testData.emissions.size(), alignedDetections.size());
            // does the final aligned detections match the emissions
            ASSERT_EQ(testData.emissions.size(), alignedDetections.size());
            ASSERT_EQ(testData.emissions, alignedDetections);
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
            std::atomic<uint32_t> alignedCalled {0};
            QubitList txIncomming;
            SequenceNumber txSeq = 0;
            QubitList rxIncomming;
            SequenceNumber rxSeq = 0;

            EXPECT_CALL(txCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, double securityParameter, QubitList* rawQubits)
            {
                txSeq = seq;
                txIncomming.insert(txIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            EXPECT_CALL(rxCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, double securityParameter, QubitList* rawQubits)
            {
                rxSeq = seq;
                rxIncomming.insert(rxIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            photons.Connect(clientChannel);
            detection.Connect(clientChannel);

            google::protobuf::Timestamp timeStamp;
            timeStamp.set_seconds(std::chrono::high_resolution_clock::now().time_since_epoch().count());

            const auto start = std::chrono::high_resolution_clock::now();

            const int itterations = 2;
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
            bool dataArrived = waitCv.wait_for(lock, std::chrono::seconds(600), [&]()
            {
                return alignedCalled == (itterations * 2);
            });

            const auto timeTaken = std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::high_resolution_clock::now() - start).count() / itterations;
            LOGINFO("Frame processing took:" + std::to_string(timeTaken) + "uS");

            detection.Disconnect();

            ASSERT_TRUE(dataArrived);
            ASSERT_GT(txSeq, 0) << "Invalid sequence number";
            ASSERT_EQ(txSeq, rxSeq) << "Sequence numbers don't match";
            ASSERT_GT(txIncomming.size(), 0) << "No data";
            ASSERT_EQ(txIncomming, rxIncomming) << "Data doesn't match";
        }

        TEST_F(AlignmentTests, DISABLED_RealData)
        {
            using namespace std::chrono;
            RandomNumber rng;
            std::unique_ptr<EmitterReport> emissions { new EmitterReport() };
            std::unique_ptr<ProtocolDetectionReport> detectionReport { new ProtocolDetectionReport() };
            ASSERT_TRUE(fs::DataFile::ReadNOXDetections("BobDetections.bin", detectionReport->detections));
            ASSERT_TRUE(fs::DataFile::ReadPackedQubits("AliceRandom.bin", emissions->emissions));

            SystemParameters params;
            params.slotWidth = nanoseconds(100);
            params.pulseWidth = nanoseconds(1);
            //params.frameWidth = emissions->emissions.size() * params.slotWidth;

            emissions->epoc = high_resolution_clock::now();
            detectionReport->epoc = emissions->epoc;

            emissions->frame = 0;
            detectionReport->frame = emissions->frame;

            emissions->period = params.slotWidth;

            align::DetectionReciever detection(params);
            align::TransmissionHandler txHandler;

            // setup classes
            grpc::ServerBuilder builder;
            int listenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
            builder.RegisterService(&txHandler);
            builder.SetMaxMessageSize(150*1024*1024);

            auto server= builder.BuildAndStart();

            grpc::ChannelArguments clientArgs;
            clientArgs.SetMaxReceiveMessageSize(-1);
            auto clientChannel = grpc::CreateCustomChannel("localhost:" + std::to_string(listenPort), grpc::InsecureChannelCredentials(), clientArgs);

            MockAlignmentCallback txCallback;
            MockAlignmentCallback rxCallback;
            detection.Attach(&rxCallback);
            txHandler.Attach(&txCallback);
            detection.Connect(clientChannel);


            std::condition_variable waitCv;
            std::mutex waitMutex;
            std::atomic<uint32_t> alignedCalled {0};
            QubitList txIncomming;
            SequenceNumber txSeq = 0;
            QubitList rxIncomming;
            SequenceNumber rxSeq = 0;

            EXPECT_CALL(txCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, double securityParameter, QubitList* rawQubits)
            {
                txSeq = seq;
                txIncomming.insert(txIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            EXPECT_CALL(rxCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([&](SequenceNumber seq, double securityParameter, QubitList* rawQubits)
            {
                rxSeq = seq;
                rxIncomming.insert(rxIncomming.end(), rawQubits->begin(), rawQubits->end());
                alignedCalled++;
                waitCv.notify_one();
            }));

            txHandler.OnEmitterReport(move(emissions));
            detection.OnPhotonReport(move(detectionReport));

            std::unique_lock<std::mutex> lock(waitMutex);
            bool dataArrived = waitCv.wait_for(lock, std::chrono::seconds(60), [&]()
            {
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
    } // namespace tests
} // namespace cqp
