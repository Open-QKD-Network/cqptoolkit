/*!
* @file
* @brief CQP Toolkit - Key storage
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "KeyManagement/keymanagement_export.h"
#include "Algorithms/Datatypes/Keys.h"
#include <grpc++/grpc++.h>

namespace cqp
{

    /// @brief Stores and manages pre-shared key
    /// Key stores are unique to a pair of endpoints.
    class KEYMANAGEMENT_EXPORT IKeyStore
    {
    public:
        /// Get the key with a specific ID. The ID should come or be derived from another IKeyStore by calling
        /// GetNewKey
        /// Once a key has been retrieved it cannot not be retrieved again
        /// @param[in] identity The identifier for a key which will be retrieved
        /// @param[out] output The key data which was retrieved
        /// @return true if the key exists
        virtual grpc::Status GetExistingKey(const KeyID& identity, PSK& output) = 0;

        /// Get a key and provide its ID
        /// Once a key has been retrieved it cannot not be retrieved again
        /// @param[out] identity The identifier for the key which was retrieved
        /// @param[out] output The key data which was retrieved
        /// @return true if a key was returned
        virtual bool GetNewKey(KeyID& identity, PSK& output) = 0;

        /**
         * @brief MarkKeyInUse
         * Prevent a key ID from being used by GetNewKey
         * @param[in] identity The key id
         * @param[out] alternative Key ID to use if there is a clash
         * @return Success or grpc::StatusCode::FAILED_PRECONDITION if the key is already in use
         *   in which case alternative will be filled with a newly reserved keyid
         */
        virtual grpc::Status MarkKeyInUse(const KeyID& identity, KeyID& alternative) = 0;

        /**
         * @brief StoreReservedKey
         * Insert a specific key which can later be extracted by calling GetExistingKey
         * @param id The key id
         * @param keyValue The key value
         * @return true on success, false if the key id is already in use.
         */
        virtual bool StoreReservedKey(const KeyID& id, const PSK& keyValue) = 0;

        /**
         * @brief SetPath
         * Change the path for this key store
         * @param path hops to use to generate keys
         * @return true if the change was accepted
         */
        virtual bool SetPath(const std::vector<std::string>& path) = 0;

        /// pure virtual destructor
        virtual ~IKeyStore() = default;
    };

}
