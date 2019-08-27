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
#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "CQPToolkit/Interfaces/IAlignmentPublisher.h"
#include <mutex>
#include <condition_variable>
#include "Algorithms/Util/DataFile.h"
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "Algorithms/Random/RandomNumber.h"
#include <grpcpp/server.h>
#include <grpcpp/channel.h>

namespace cqp
{
    namespace tests
    {

        /**
         * @brief The MockAlignmentCallback class
         * Mock method for receiving output from a publisher
         */
        class MockAlignmentCallback: public virtual ISiftedCallback
        {
        public:
            /// dummy callback for reacting to test data
            MOCK_METHOD3(OnSifted2, void(SequenceNumber seq, double securityParameter, JaggedDataBlock* rawQubits));
            void OnSifted(SequenceNumber seq, double securityParameter, std::unique_ptr<JaggedDataBlock> siftedData) override
            {
                OnSifted2(seq, securityParameter, siftedData.get());
            }
        };

        /**
         * @brief The MockAlignmentCallback class
         * Mock method for receiving output from a publisher
         */
        class MockTxCallback: public remote::IAlignment::Service
        {
        public:
            // Service interface
        public:
            MOCK_METHOD3(GetAlignmentMarks, grpc::Status(grpc::ServerContext *, const remote::MarkersRequest* request, remote::MarkersResponse *response));
            MOCK_METHOD3(DiscardTransmissions, grpc::Status(grpc::ServerContext *, const remote::ValidDetections *request, google::protobuf::Empty *));
        };

        /**
         * @test
         * @brief Alignment::Alignment
         * Base class for testing alignment interface
         */
        class AlignmentTests : public testing::Test
        {
        public:
            /**
             * @brief Alignment
             * Constructor
             */
            AlignmentTests();

        protected:
            /// dummy callback for reacting to test data
            MockAlignmentCallback aliceCallback;
            /// dummy callback for reacting to test data
            MockAlignmentCallback bobCallback;

            /// mutex for access to test results
            std::mutex m;
            /// for access to test results
            std::condition_variable cv;

            MockTxCallback txCallback;
            std::unique_ptr<grpc::Server> server;
            std::shared_ptr<grpc::Channel> clientChannel;
            std::shared_ptr<RandomNumber> rng;
        };

    } // namespace tests
} // namespace cqp


