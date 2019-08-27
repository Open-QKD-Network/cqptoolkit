/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/8/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <IDQDevices/Clavis3/ClavisKeyFile.h>
#include <thread>
#include <Algorithms/Random/RandomNumber.h>
#include <Algorithms/Logging/ConsoleLogger.h>
#include "Algorithms/Util/FileIO.h"

namespace cqp
{
    TEST(Calvis3, FileReader)
    {
        using namespace std;
        const auto filename = "test.dat";

        fs::Delete(filename);

        ConsoleLogger::Enable();
        DefaultLogger().SetOutputLevel(LogLevel::Trace);

        ClavisKeyFile reader(filename);

        this_thread::sleep_for(chrono::seconds(1));

        auto output = ofstream(filename);
        this_thread::sleep_for(chrono::seconds(1));

        RandomNumber rng;
        DataBlock buffer;
        rng.RandomBytes(48*5, buffer);

        output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        output.flush();
        this_thread::sleep_for(chrono::seconds(1));

        buffer.clear();

        for(auto count = 0u; count < 5; count++)
        {
            rng.RandomBytes(13, buffer);

            output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            output.flush();
        }
        output.close();
        fs::Delete(filename);
    }
}
