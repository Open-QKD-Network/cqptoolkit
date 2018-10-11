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
#include "TestEvent.h"
#include <thread>
#include <chrono>

namespace cqp
{
    namespace tests
    {

        ACTION_P2(ReturnFromAsyncCall, somethinghappened, cv)
        {
            *somethinghappened = true;
            cv->notify_one();
        }

        /**
         * @test
         * @brief TEST
         */
        TEST(EventTest, CheckEmit)
        {
            std::mutex mtx;
            std::condition_variable cv;
            bool callCalled = false;

            MockListener cb;
            MockEvent event;
            int data = 41;
            EXPECT_CALL(cb, Callback(42)).Times(1)
            .WillRepeatedly(ReturnFromAsyncCall(&callCalled, &cv));

            event.Emit(data);
            data = 42;
            event.Add(&cb);
            event.Emit(data);
            event.Remove(&cb);
            data = 43;
            event.Emit(data);

            {

                std::unique_lock<std::mutex> lock(mtx);
                EXPECT_TRUE(cv.wait_for(
                                lock,
                                std::chrono::seconds(2),
                                [&callCalled]()
                {
                    return callCalled;
                }
                            )
                           );
            }
        }
    }
}
