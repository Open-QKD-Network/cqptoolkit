/*!
* @file
* @brief main.cpp
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 04/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "gtest/gtest.h"
#include "CQPToolkit/Util/ConsoleLogger.h"

int main(int argc, char **argv)
{
    cqp::ConsoleLogger::Enable();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
