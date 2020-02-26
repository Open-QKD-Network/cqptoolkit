/*!
* @file
* @brief Remoting Tests
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22/8/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <mutex>
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"
#include <condition_variable>
#include <gtest/gtest-param-test.h>

namespace cqp
{
    namespace tests
    {

        /**
         * @brief The MockSiftCallback class
         * Mock method for receiving output from a publisher
         */
        class MockSiftCallback: public virtual ISiftedCallback
        {
        public:
            /**
             * @brief MOCK_METHOD2
             */
            MOCK_METHOD3(OnSifted2, void(
                             const SequenceNumber id,
                             double securityParameter,
                             const JaggedDataBlock* siftedData));
            void OnSifted(const SequenceNumber id,
                          double securityParameter,
                          std::unique_ptr<JaggedDataBlock> siftedData) override {
                OnSifted2(id, 0.0, siftedData.get());
            }
        };

        /**
         * @test
         * @brief The Sift class
         * For testing sifting implementations
         * @details
         * Base class for testing sifting code
         */
        class SiftTests : public testing::TestWithParam<bool>
        {
        public:
            /**
             * @brief Sift
             * Constructor
             */
            SiftTests();

            virtual ~SiftTests() {}

        protected:
            /// dummy callback for reacting to test data
            MockSiftCallback aliceCallback;
            /// dummy callback for reacting to test data
            MockSiftCallback bobCallback;


            /// mutex for access to test results
            std::mutex m;
            /// for access to test results
            std::condition_variable cv;
        };

        INSTANTIATE_TEST_CASE_P(ParamedSifTest, SiftTests, testing::Bool());

    } // namespace tests
} // namespace cqp
