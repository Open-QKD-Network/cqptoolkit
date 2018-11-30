/*!
* @file
* @brief AlignmentTestData
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestSerial.h"
#include "CQPToolkit/Drivers/Serial.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include <string>

namespace cqp
{
    namespace tests
    {

        /// This test requires a pair of virtual serial ports which act like a loopback
        /// On windows this is accomplished with (com0com)[https://code.google.com/archive/p/powersdr-iq/downloads]
        /// It assumed that the first two matching serial ports are a pair and tries to use them.
        /// "emulate baud rate" should be enabled in the setup.
        ///
        /// To run these tests on Linux, install the socat utility
        TEST_F(TestSerial, EnumerationTest)
        {
            ConsoleLogger::Enable();
            SerialList devices;
            Serial unit;

            const char bigData[] = "asdaklsdwdqoiwqoiwdjqoiwdjadja;slkdjlkajdwqo";

            ASSERT_NO_THROW(Serial::Detect(devices, false)) << " Testing detection";

            // these are hard coded because they need to be paired
            // Enumeration can come back in any order
            Serial left("\\\\.\\COM3");
            Serial right("\\\\.\\COM4");

            if(left.Open() && right.Open())
            {

                char buffer[sizeof(bigData)] = { 0 };
                uint32_t buffSize = 1;

                ASSERT_TRUE(left.Write('A'));
                buffSize = 1;
                ASSERT_TRUE(right.Read(buffer, buffSize));
                ASSERT_EQ(buffer[0], 'A');

                ASSERT_TRUE(left.Write(13));
                buffSize = 1;
                ASSERT_TRUE(right.Read(buffer, buffSize));
                ASSERT_EQ(buffer[0], 13);

                ASSERT_TRUE(left.Write('\xFF'));
                buffSize = 1;
                ASSERT_TRUE(right.Read(buffer, buffSize));
                ASSERT_EQ(buffer[0], '\xFF');

                buffer[0] = 0;
                ASSERT_TRUE(left.Write(bigData, buffSize));
                buffSize = sizeof(buffer);
                ASSERT_TRUE(right.Read(buffer, buffSize));
                ASSERT_STREQ(buffer, bigData);
            }
            else
            {
                LOGINFO("No virtual com port, transfer tests skipped.");
            }

            ASSERT_TRUE(left.Close());
            ASSERT_TRUE(right.Close());
        }

    }
}
