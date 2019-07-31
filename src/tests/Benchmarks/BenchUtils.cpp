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
#include "BenchUtils.h"
#include "Algorithms/Util/Hash.h"
#include "benchmark/benchmark.h"

namespace cqp
{
    namespace tests
    {
        static void BM_FNV1aHash(benchmark::State& state)
        {

            for(auto _ : state)
            {
                FNV1aHash("Some amount of data which isn't very small");
            }
        }

        BENCHMARK(BM_FNV1aHash);

    } // namespace tests
} // namespace cqp
