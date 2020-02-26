/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiftTests.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Sift/Receiver.h"
#include "CQPToolkit/Sift/Verifier.h"

#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include <gmock/gmock-actions.h>

namespace cqp
{
    namespace tests
    {

        using ::testing::_;
        using namespace ::testing;

        SiftTests::SiftTests()
        {
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Debug);
        }

        TEST_P(SiftTests, Sifting)
        {
            using namespace cqp::sift;
            using namespace std;

            Verifier reciever;

            // Setup server
            using namespace grpc;

            // Send data to the input
            SequenceNumber seq = 0;

            QubitList data
            {
                4, 4, 4, 2, 1, 4, 5, 2, 4, 2,
                5, 3, 1, 1, 4, 2, 4, 3, 5, 3,
                4, 1, 2, 2, 5, 2, 1, 5, 5, 5,
                3, 2, 3, 1, 5, 2, 2, 1, 2, 3,
                5, 4, 1, 5, 2, 3, 5, 4, 5, 2,
                2, 3, 4, 3, 1, 1, 3, 3, 1, 3,
                3, 1, 5, 3, 2, 3, 5, 2, 1, 4,
                2, 2, 5, 5, 4, 1, 1, 4, 1, 2,
                3, 3, 2, 5, 5, 5, 2, 5, 1, 3,
                2, 2, 2, 3, 2, 2, 4, 1, 2, 3
            };

            const std::set<Intensity> discardIntensities = {0};
            std::unique_ptr<IntensityList> intensities;
            const bool useIntensities = GetParam();
            size_t numberDiscarded = 0;

            if(useIntensities)
            {
                LOGINFO("Testing with multiple intensities");
                intensities = make_unique<IntensityList>();
                *intensities =
                {
                    0,  1,  2,  2,  3,  2,  2,  0,  0,  1,
                    3,  1,  1,  2,  3,  3,  2,  0,  0,  0,
                    0,  0,  2,  2,  2,  3,  1,  0,  1,  1,
                    3,  0,  0,  3,  0,  1,  0,  1,  1,  1,
                    1,  3,  1,  3,  1,  2,  2,  1,  1,  2,
                    1,  3,  0,  2,  1,  2,  3,  2,  3,  2,
                    1,  2,  0,  3,  3,  3,  3,  0,  0,  0,
                    3,  1,  1,  1,  3,  2,  0,  1,  2,  0,
                    2,  1,  2,  0,  2,  1,  1,  0,  0,  0,
                    0,  3,  2,  0,  1,  2,  1,  2,  2,  2
                };
                numberDiscarded = std::count_if(intensities->begin(), intensities->end(), [&](auto val)
                {
                    return std::find(discardIntensities.begin(), discardIntensities.end(), val) != discardIntensities.end();
                });
            }


            // Results
            JaggedDataBlock aliceResults;
            JaggedDataBlock bobResults;

            // how to handle callbacks
            EXPECT_CALL(aliceCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([this, &aliceResults](const SequenceNumber, double securityParameter,
                    const JaggedDataBlock* siftedData)
            {
                {
                    lock_guard<mutex> lock(m);
                    aliceResults = *siftedData;
                }
                cv.notify_one();
            }));

            EXPECT_CALL(bobCallback, OnSifted2(_, _, _)).WillRepeatedly(Invoke([this, &bobResults](const SequenceNumber, double securityParameter,
                    const JaggedDataBlock* siftedData)
            {
                {
                    lock_guard<mutex> lock(m);
                    bobResults = *siftedData;
                }
                cv.notify_one();
            }));

            // Change the data
            QubitList touched = data;
            const vector<pair<int, int>> changes
            {
                {   0 + 0, 2},
                {   2 + 0, 3},
                {   4 + 1, 3},
                {   6 + 1, 5},
                {   8 + 0, 3},
                {  10 + 0, 2},
                {  12 + 0, 5},
                {  14 + 1, 0},
                {  16 + 0, 1},
                {  18 + 1, 4},
                {  20 + 0, 1},
                {  22 + 0, 1},
                {  24 + 0, 3},
                {  26 + 1, 3},
                {  28 + 1, 1},
                {  30 + 0, 1},
                {  32 + 0, 4},
                {  34 + 1, 1},
                {  36 + 0, 4},
                {  38 + 0, 4},
                {  40 + 1, 3},
                {  42 + 0, 3},
                {  44 + 0, 1},
                {  46 + 0, 3},
                {  48 + 1, 1},
                {  50 + 0, 4},
                {  52 + 1, 5},
                {  54 + 1, 2},
                {  56 + 0, 5},
                {  58 + 0, 3},
                {  60 + 0, 5},
                {  62 + 0, 2},
                {  64 + 0, 4},
                {  66 + 1, 1},
                {  68 + 1, 1},
                {  70 + 0, 0},
                {  72 + 1, 3},
                {  74 + 0, 1},
                {  76 + 1, 3},
                {  78 + 0, 4},
                {  80 + 0, 4},
                {  82 + 1, 1},
                {  84 + 0, 0},
                {  86 + 0, 1},
                {  88 + 1, 4},
                {  90 + 1, 1},
                {  92 + 0, 4},
                {  94 + 0, 5},
                {  96 + 1, 3},
                {  98 + 0, 1}
            };

            const ulong changesMade = changes.size();
            for(auto mod : changes)
            {
                ASSERT_NE(data[mod.first], mod.second) << "Change value == original, check " + std::to_string(mod.first);
                ASSERT_NE(QubitHelper::Base(data[mod.first]), QubitHelper::Base(mod.second)) <<
                        "Not changing the base from "
                        << std::to_string((int)QubitHelper::Base(data[mod.first]))
                        << ", check " + std::to_string(mod.first);
                touched[mod.first] = mod.second;
            }

            /// Number of bytes produced
            const unsigned long numGoodBytes = ceil(static_cast<float>((data.size() - changesMade)) / 8.0);

            // setup classes
            ServerBuilder builder;
            int listenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
            builder.RegisterService(&reciever);

            std::unique_ptr<Server> server(builder.BuildAndStart());

            ASSERT_NE(server, nullptr);
            std::shared_ptr<Channel> clientChannel = grpc::CreateChannel("localhost:" + std::to_string(listenPort), grpc::InsecureChannelCredentials());
            ASSERT_NE(clientChannel, nullptr);

            Receiver transitter;
            transitter.Connect(clientChannel);

            // craete copies of the data to move on
            auto emitterReport = make_unique<EmitterReport>();
            emitterReport->emissions = data;

            if(useIntensities)
            {
                // discard all qubits with intensity of 0
                reciever.SetDiscardedIntensities(discardIntensities);
                transitter.SetDiscardedIntensities(discardIntensities);
                emitterReport->intensities = *intensities;
            }
            // add callbacks
            reciever.Attach(&aliceCallback);
            transitter.Attach(&bobCallback);

            auto photonReport = make_unique<ProtocolDetectionReport>();
            photonReport->detections.resize(touched.size());
            for(size_t index = 0; index < data.size(); index++)
            {
                photonReport->detections[index] =
                {
                    PicoSeconds(index), touched[index]
                };
            }

            reciever.OnEmitterReport(move(emitterReport));
            transitter.OnPhotonReport(move(photonReport));
            seq++;

            bool gotLock = false;
            {
                unique_lock<mutex> lock(m);

                gotLock = cv.wait_for(lock, chrono::seconds(5), [&]()
                {
                    return (!aliceResults.empty() && !bobResults.empty());
                });
            }

            if(gotLock )
            {
                cout << "Alice:" << uppercase << hex;
                for(auto byte : aliceResults)
                {
                    cout << setw(sizeof(byte) * 2) << setfill('0') << uppercase << hex << +byte;
                }
                cout << endl;
                cout << "Bob  :" << uppercase << hex;
                for(auto byte : bobResults)
                {
                    cout << setw(sizeof(byte) * 2) << setfill('0') << uppercase << hex << +byte;
                }
                cout << endl;
                ASSERT_NE(aliceResults, data) << "Test invalid. No errors removed.";
                if(!useIntensities)
                {
                    // using intensities discards more but those bits may have been discarded by the sifting process anyway.
                    ASSERT_EQ(aliceResults.size(), numGoodBytes) << "Wrong number of bytes returned";
                }
                ASSERT_EQ(aliceResults, bobResults) << "Results do not match";
            }
            else
            {
                FAIL() << "No data received before timeout";
            }
        }

    } // namespace tests
} // namespace cqp
