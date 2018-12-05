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
#include "Algorithms/Datatypes/RingBuffer.h"
#include "gtest/gtest.h"

namespace cqp
{
    namespace tests
    {
        /**
         * @test
         * @brief The TestRingBuffer class
         * Test the rung buffer
         */
        class TestRingBuffer : public testing::Test
        {
        public:
            /**
             * @brief TestRingBuffer
             */
            TestRingBuffer();
        protected:
            /// unit under test
            RingBuffer<int, 3> unit;
        };
    }
}
