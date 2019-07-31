/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 31/7/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "benchmark/benchmark.h"

#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Random/RandomNumber.h"

namespace cqp
{
    namespace tests
    {
        static void BM_StoreKeyInFileStore(benchmark::State& state)
        {
            const std::string dest = "SiteB";
            keygen::IBackingStore::Keys keyData;
            PSK aKey;

            RandomNumber rng;
            rng.RandomBytes(32, aKey);
            keyData.push_back({1, aKey});


            fs::Delete("FileStoreTest.db");
            keygen::FileStore fileStore("FileStoreTest.db");

            for(auto _ : state)
            {
                fileStore.StoreKeys(dest, keyData);

                keyData[0].first++;
            }
        }
        BENCHMARK(BM_StoreKeyInFileStore);

        static void BM_RetrieveKeyFromFileStore(benchmark::State& state)
        {
            const std::string dest = "SiteB";
            keygen::IBackingStore::Keys keyData;
            PSK aKey;

            RandomNumber rng;
            rng.RandomBytes(32, aKey);

            fs::Delete("FileStoreTest.db");
            keygen::FileStore fileStore("FileStoreTest.db");

            for(auto i = 0u; i < state.max_iterations; i++)
            {
                keyData.push_back({i, aKey});
            }
            fileStore.StoreKeys(dest, keyData);

            KeyID id = 0;

            for(auto _ : state)
            {
                fileStore.RemoveKey(dest, id++, aKey);
            }
        }
        BENCHMARK(BM_RetrieveKeyFromFileStore);
    } // namespace tests
} // namespace cqp
