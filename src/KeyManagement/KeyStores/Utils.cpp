/*!
* @file
* @brief Utils
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/9/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Utils.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Random/RandomNumber.h"

namespace cqp
{
    namespace keygen
    {

        Utils::Utils()
        {
        }

        bool Utils::PopulateRandom(const std::string& leftId, IBackingStore& leftStore,
                                   const std::string& rightId, IBackingStore& rightStore,
                                   uint64_t numberKeysToAdd, uint16_t keyBytes)
        {
            bool result = false;
            auto nextId = leftStore.GetNextKeyId(rightId);
            const auto nextRightId = rightStore.GetNextKeyId(leftId);

            if(nextRightId == nextId)
            {
                LOGTRACE("Key Ids match");
                RandomNumber rng;
                IBackingStore::Keys keys;
                keys.resize(numberKeysToAdd);

                // generate the key values
                for(uint64_t count = 0; count < numberKeysToAdd; count++)
                {
                    keys[count].first = nextId;
                    rng.RandomBytes(keyBytes, keys[count].second);

                    nextId++;
                }

                // copy the keys as storing them destroys them
                auto rightKeys = keys;
                result = leftStore.StoreKeys(rightId, keys);
                result &= rightStore.StoreKeys(leftId, rightKeys);

                result &= keys.empty() && rightKeys.empty();
                if(!result)
                {
                    LOGERROR("Failed to store keys");
                }

                size_t available = 0;
                size_t capacity = 0;

                leftStore.GetCounts(rightId, available, capacity);
                LOGINFO(leftId + " <-> " + rightId + " has " + std::to_string(available) + " keys available");
            }
            else
            {
                LOGERROR("Keystores " + leftId + " and " + rightId + " are out of sync: " + std::to_string(nextId) + " vs " + std::to_string(nextRightId));
                result = false;
            }

            return result;
        }

        bool Utils::PopulateRandom(const Utils::KeyStores& stores, uint64_t numberKeysToAdd, uint16_t keyBytes)
        {
            bool result = true;

            for(size_t fromIndex = 0; fromIndex < stores.size(); fromIndex++)
            {
                const auto& from = stores[fromIndex];

                for(size_t toIndex = fromIndex + 1; toIndex < stores.size(); toIndex++)
                {
                    const auto& to = stores[toIndex];
                    result &= PopulateRandom(from.first, *from.second, to.first, *to.second,
                                             numberKeysToAdd, keyBytes);
                }
            }

            return result;
        }

    } // namespace keygen
} // namespace cqp
