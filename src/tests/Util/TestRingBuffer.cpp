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
#include "TestRingBuffer.h"


namespace cqp
{
    namespace tests
    {

        TestRingBuffer::TestRingBuffer()
        {
            unit.Push(42);
            unit.Push(123);
            unit.Push(-1);
        }

        TEST_F(TestRingBuffer, UnitTest1)
        {
            ASSERT_EQ(unit.Pop(), 42);
            ASSERT_EQ(unit.Pop(), 123);
            ASSERT_EQ(unit.Pop(), -1);
        }
    }
}