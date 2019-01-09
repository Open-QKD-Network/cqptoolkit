/*!
* @file
* @brief main
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "gtest/gtest.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "KeyManagement/KeyStores/PKCS11Wrapper.h"
int main(int argc, char **argv)
{
    cqp::ConsoleLogger::Enable();
    cqp::DefaultLogger().SetOutputLevel(cqp::LogLevel::Debug);
    if(!cqp::p11::Module::Create("libsofthsm2.so"))
    {
        LOGWARN("Disabling PKCS tests due to missing driver.");
        testing::GTEST_FLAG(filter) = "-KeyMan.HSM*";
    }
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
