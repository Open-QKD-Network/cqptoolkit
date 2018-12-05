/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 3/7/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Keys.h"
#include "KeyManagement/keymanagement_export.h"

namespace cqp
{

    namespace keygen
    {
        /**
         * @brief The IBackingStore interface
         * for storing and retrieving keys. All methods assume that the keys are store for this location for
         * another destination, not between two arbitrary points
         */
        class KEYMANAGEMENT_EXPORT IBackingStore
        {
        public:
            /// a key with it's id
            using Key = std::pair<KeyID, PSK>;
            /// storage for keys and their ids
            using Keys = std::vector<Key>;

            /**
             * @brief StoreKeys
             * Put keys into storage
             * @details
             * The keys parameter will be modified so that if any keys don't reach the backing store,
             * maybe because of lack of space, they will remain in the keys list.
             * @param[in] destination The far end point which these keys have been shared with
             * @param[in, out] keys The keys to store
             * @return true on success
             */
            virtual bool StoreKeys(const std::string& destination, Keys& keys) = 0;

            /**
             * @brief RemoveKey
             * Extract the key value and delete the key from storage.
             * @param[in] destination The far end point which these keys have been shared with
             * @param[in] keyId The id of the key to extract
             * @param[out] output The key value if the key was extracted
             * @return true on success
             */
            virtual bool RemoveKey(const std::string& destination, KeyID keyId, PSK& output) = 0;

            /**
             * @brief RemoveKeys
             * Extract the key values and delete the keys from storage.
             * @param[in] destination The far end point which these keys have been shared with
             * @param[in,out] keys A list of storage locations for the keys
             * @return true on success
             */
            virtual bool RemoveKeys(const std::string& destination, Keys& keys) = 0;

            /**
             * @brief ReserveKey
             * Returns a key id which is not in use so that it can later be retrieved with RemoveKey.
             * Subsequent calls to this function should not return the same id
             * @param[in] destination The far end point which these keys have been shared with
             * @param[out] keyId The id of the free key.
             * @return true on success
             */
            virtual bool ReserveKey(const std::string& destination, KeyID& keyId) = 0;

            /**
             * @brief GetCounts
             * Returns the usage of the store
             * @param[in] destination The far end point which these keys have been shared with
             * @param[out] availableKeys How many keys are currently in storage
             * @param[out] remainingCapacity How many more bytes can be stored (or std::numeric_limits<uint64_t>::max() for unlimited)
             */
            virtual void GetCounts(const std::string& destination, uint64_t& availableKeys, uint64_t& remainingCapacity) = 0;

            /**
             * @brief GetNextKeyId
             * @param[in] destination The far end point which these keys have been shared with
             * @return The next key id which has not been used.
             */
            virtual uint64_t GetNextKeyId(const std::string& destination) = 0;

            /// default destructor
            virtual ~IBackingStore() = default;
        };
    }
}
