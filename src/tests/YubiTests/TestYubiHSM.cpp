/*!
* @file
* @brief TestKeyMan
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 9/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestYubiHSM.h"
#include "KeyManagement/KeyStores/YubiHSM.h"
#include "Algorithms/Util/Process.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPUI/OpenSSLKeyUI.h"
#include <QApplication>
#include "Algorithms/Util/Strings.h"
#define YH_ALGO_OPAQUE_DATA 30

namespace cqp
{
    namespace tests
    {
        TEST(YubiHSM, DISABLED_ExtractRawKey)
        {
            using namespace p11;
            using namespace std;
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Trace);

            const keygen::IBackingStore::Key key {123,
                {
                    185, 182, 156, 211,  87, 183,  52, 248,  47, 214, 120, 101,  47,  71, 154, 186,
                    103,  36, 132, 218, 119, 190,  28, 185,  89, 168,  29, 124,  29, 211, 132, 210
                }};
            //const std::string loadOptions = "connect=http://localhost:12345\ndebug\nlibdebug\ndinout";

            //Process connectorProc;
            //ASSERT_TRUE(connectorProc.Start("/usr/bin/yubihsm-connector"));

            ASSERT_EQ(key.second.size(), 32);

            keygen::IBackingStore::Keys keys {key};
            keygen::YubiHSM hsm("pkcs11:module-name=yubihsm_pkcs11.so?pin-value=0001password");

            const std::string destination = "YubiHSM-Test";

            PSK keyOut;
            if(hsm.KeyExists(destination, key.first))
            {
                hsm.RemoveKey(destination, key.first, keyOut);
                keyOut.clear();
            }
            ASSERT_TRUE(hsm.StoreKeys(destination, keys));

            ASSERT_TRUE(hsm.RemoveKey(destination, key.first, keyOut));

            ASSERT_EQ(key.second, keyOut);

            //connectorProc.RequestTermination(true);
        }

    }
}
