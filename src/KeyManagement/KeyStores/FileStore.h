/*!
* @file
* @brief FileStore
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/6/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(SQLITE3_FOUND)
#include <bits/stdint-uintn.h>                    // for uint64_t
#include <mutex>                                  // for mutex
#include <string>                                 // for string
#include "Algorithms/Datatypes/Keys.h"            // for KeyID
#include "KeyManagement/KeyStores/IBackingStore.h"
#include "KeyManagement/keymanagement_export.h"

struct sqlite3;
struct sqlite3_stmt;

namespace cqp
{
    namespace keygen
    {

        /**
         * @brief The FileStore class
         * Use a database to store the keys
         */
        class KEYMANAGEMENT_EXPORT FileStore : public virtual IBackingStore
        {
        public:
            /**
             * @brief FileStore
             * Constructor
             * @param filename The file to load and store keys from
             */
            explicit FileStore(const std::string& filename = ":memory:");
            /// Destructor
            ~FileStore() override;

            /// @{
            ///@name  IBackingStore interface

            /// @copydoc IBackingStore::StoreKeys
            bool StoreKeys(const std::string& destination, Keys& keys) override;

            /// @copydoc IBackingStore::RemoveKey
            bool RemoveKey(const std::string& destination, KeyID keyId, PSK& output) override;

            /// @copydoc IBackingStore::RemoveKeys
            bool RemoveKeys(const std::string& destination, Keys& keys) override;

            /// @copydoc IBackingStore::ReserveKey
            bool ReserveKey(const std::string& destination, KeyID& keyId) override;

            /// @copydoc IBackingStore::GetCounts
            void GetCounts(const std::string& destination, uint64_t& availableKeys, uint64_t& remainingCapacity) override;
            /// @copydoc IBackingStore::GetNextKeyId
            virtual uint64_t GetNextKeyId(const std::string& destination) override;
            ///@}
        protected: // members
            /// The database object
            sqlite3* db = nullptr;
            /// prepared statement to insert a key
            sqlite3_stmt* insertStmt = nullptr;
            /// prepared statement to get a key
            sqlite3_stmt* getKeyStmt = nullptr;
            /// prepared statement to get an unused key
            sqlite3_stmt* getAvailableKeyStmt = nullptr;
            /// prepared statement to marka  key as in use so getAvailableKeyStmt will ignore it
            sqlite3_stmt* markInUseStmt = nullptr;
            /// prepared statement to remove a key
            sqlite3_stmt* deleteKeyStmt = nullptr;
            /// prepared statement to add a mapping from destination to link id
            sqlite3_stmt* insertLinkStmt = nullptr;
            /// prepared statement to count keys in the database
            sqlite3_stmt* countKeysStmt = nullptr;
            /// prepared statement to update the next key id
            sqlite3_stmt* getNextIdStmt = nullptr;

            /// protect against the select statement being used my multiple threads
            std::mutex queryMutex;

        protected: // methods

            /**
             * @brief CheckSQLite
             * If the result from a sqlite command is an error - print the error message
             * @param retCode result of sqlite command
             * @return retCode
             */
            static int CheckSQLite(int retCode);

            /**
             * @brief SQLiteOk
             * If the result from a sqlite command is an error - print the error message and return false
             * @param retCode
             * @return true if the command succeeded
             */
            static bool SQLiteOk(int retCode);
        };
    } // namespace keygen
} // namespace cqp
#endif // sqlite found
