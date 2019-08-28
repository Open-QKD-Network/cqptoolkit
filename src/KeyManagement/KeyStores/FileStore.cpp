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
#include "FileStore.h"
#if defined(SQLITE3_FOUND)
#include <sqlite3.h>                   // for sqlite3_bind_int64, sqlite3_exec
#include <limits>                      // for numeric_limits
#include <utility>                     // for pair
#include "Algorithms/Util/Hash.h"      // for FNV1aHash
#include "Algorithms/Datatypes/Keys.h"            // for PSK, KeyID
#include "KeyManagement/KeyStores/IBackingStore.h"  // for IBackingStore::Keys
#include "Algorithms/Logging/Logger.h"               // for LOGERROR

namespace cqp
{
    namespace keygen
    {

        int FileStore::CheckSQLite(int retCode)
        {
            if(retCode != SQLITE_OK && retCode != SQLITE_DONE && retCode != SQLITE_ROW)
            {
                LOGERROR(sqlite3_errstr(retCode));
            }
            return retCode;
        }

        bool FileStore::SQLiteOk(int retCode)
        {
            bool result = true;
            if(retCode != SQLITE_OK && retCode != SQLITE_DONE && retCode != SQLITE_ROW)
            {
                result = false;
                LOGERROR(sqlite3_errstr(retCode));
            }
            return result;
        }

        FileStore::FileStore(const std::string& filename)
        {
            // Note: binds are indexed from 1, columns are indexed from 0
            // open the database
            auto result = CheckSQLite(sqlite3_open(filename.c_str(), &db));

            // set the busy timeout
            CheckSQLite(sqlite3_busy_timeout(db, 1000));

            if(result == SQLITE_OK)
            {
                const std::string stmt = "PRAGMA writable_schema = 1;"
                                         "PRAGMA TEMP_STORE=MEMORY;"
                                         "PRAGMA JOURNAL_MODE=WAL;"
                                         "PRAGMA SYNCHRONOUS=OFF;" // may loose data on power failure, use NORMAL with WAL for perfect safety (slower)
                                         "PRAGMA SECURE_DELETE=FAST;"
                                         "BEGIN TRANSACTION;"
                                         "CREATE TABLE IF NOT EXISTS `keys` ("
                                         "        `LinkID`	INTEGER NOT NULL,"
                                         "        `ID`	INTEGER NOT NULL,"
                                         "        `Value`	BLOB NOT NULL,"
                                         "        `InUse`       INTEGER DEFAULT 0,"
                                         "        PRIMARY KEY(`ID`,`LinkID`)"
                                         ");"
                                         "CREATE TABLE IF NOT EXISTS `links` ("
                                         "        `LinkID`	INTEGER NOT NULL UNIQUE,"
                                         "        `SiteB`	TEXT NOT NULL UNIQUE,"
                                         "        `NextKeyID`   INTEGER DEFAULT 1,"
                                         "        PRIMARY KEY(`LinkID`)"
                                         ");"
                                         "COMMIT;"
                                         "PRAGMA OPTIMIZE;"
                                         "PRAGMA writable_schema = 0;";

                // create the schema
                result = CheckSQLite(sqlite3_exec(db, stmt.c_str(), nullptr, nullptr, nullptr));
            }

            // create statements
            if(result == SQLITE_OK)
            {
                std::string command = "INSERT INTO keys (LinkID, ID, Value)"
                                      " VALUES (?, ?, ?)";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &insertStmt, nullptr));

                command = "INSERT OR IGNORE INTO links (LinkID, SiteB) VALUES (?, ?)";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &insertLinkStmt, nullptr));

                command = "SELECT (Value) FROM keys where LinkID = ? AND ID = ?";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &getKeyStmt, nullptr));

                command = "SELECT (ID) FROM keys WHERE (LinkID = ?1 AND InUse = 0) ORDER BY ID LIMIT 1";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &getAvailableKeyStmt, nullptr));

                command = "UPDATE OR FAIL keys SET InUse = 1 WHERE (LinkID = ?1 AND ID = ?2)";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &markInUseStmt, nullptr));

                command = "DELETE FROM keys WHERE LinkID = ?1 AND ID = ?2";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &deleteKeyStmt, nullptr));

                command = "SELECT COUNT(*) FROM keys WHERE LinkID = ?";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &countKeysStmt, nullptr));

                command = "UPDATE links SET NextKeyID = (SELECT max(ID)+1 AS NextID FROM keys) WHERE LinkID = ?";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &updateNextIdStmt, nullptr));

                command = "SELECT NextKeyID FROM links WHERE LinkID = ?";
                result |= CheckSQLite(sqlite3_prepare_v2(db, command.c_str(), command.length(), &getNextIdStmt, nullptr));

                // if you add a statement, add a finalize call to the destructor
            }

            if(result != SQLITE_OK)
            {
                LOGERROR("Error creating database");
            }
        }

        FileStore::~FileStore()
        {
            // perform a vacuum to de-fragment and shrink the database file.
            CheckSQLite(sqlite3_exec(db, "PRAGMA OPTIMIZE; VACUUM;", nullptr, nullptr, nullptr));
            // cleanup statements
            CheckSQLite(sqlite3_finalize(insertStmt));
            CheckSQLite(sqlite3_finalize(getKeyStmt));
            CheckSQLite(sqlite3_finalize(getAvailableKeyStmt));
            CheckSQLite(sqlite3_finalize(markInUseStmt));
            CheckSQLite(sqlite3_finalize(deleteKeyStmt));
            CheckSQLite(sqlite3_finalize(insertLinkStmt));
            CheckSQLite(sqlite3_finalize(countKeysStmt));
            CheckSQLite(sqlite3_finalize(updateNextIdStmt));
            CheckSQLite(sqlite3_finalize(getNextIdStmt));

            CheckSQLite(sqlite3_close(db));
        }

        bool FileStore::StoreKeys(const std::string& destination, Keys& keys)
        {
            // Note: binds are indexed from 1, columns are indexed from 0
            // hash the destination to get the link id
            const uint64_t link = FNV1aHash(destination);

            // start a transaction
            // suspect using a prepared statement he would cause issues with multiple threads
            CheckSQLite(sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr));
            // setup the link table insert
            CheckSQLite(sqlite3_bind_int64(insertLinkStmt, 1, link));
            CheckSQLite(sqlite3_bind_text(insertLinkStmt, 2, destination.c_str(), destination.length(), SQLITE_TRANSIENT));

            // add the destination/link id if it doesn't already exist
            bool result = SQLiteOk(sqlite3_step(insertLinkStmt));
            CheckSQLite(sqlite3_reset(insertLinkStmt));

            for(auto key : keys)
            {
                // load the incoming values in to the statement
                CheckSQLite(sqlite3_bind_int64(insertStmt, 1, link));
                CheckSQLite(sqlite3_bind_int64(insertStmt, 2, key.first));
                CheckSQLite(sqlite3_bind_blob(insertStmt, 3, key.second.data(), key.second.size(), nullptr));
                // store the key
                result &= SQLiteOk(sqlite3_step(insertStmt));
                CheckSQLite(sqlite3_reset(insertStmt));
            } // for keys

            // update the next id field with the highest number in the table
            CheckSQLite(sqlite3_bind_int64(updateNextIdStmt, 1, link));
            CheckSQLite(sqlite3_step(updateNextIdStmt));
            CheckSQLite(sqlite3_reset(updateNextIdStmt));

            // commit the transaction
            CheckSQLite(sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr));
            keys.clear();

            return result;
        } // StoreKeys

        bool FileStore::RemoveKey(const std::string& destination, KeyID keyId, PSK& output)
        {
            // Note: binds are indexed from 1, columns are indexed from 0
            bool result = false;
            const uint64_t link = FNV1aHash(destination);
            bool doCommit = false;

            // from docs: After a BEGIN IMMEDIATE, no other database connection will be able to write to the database or do a BEGIN IMMEDIATE or BEGIN EXCLUSIVE. Other processes can continue to read from the database, however.
            CheckSQLite(sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr));

            CheckSQLite(sqlite3_bind_int64(getKeyStmt, 1, link));
            CheckSQLite(sqlite3_bind_int64(getKeyStmt, 2, keyId));
            if(CheckSQLite(sqlite3_step(getKeyStmt)) == SQLITE_ROW)
            {
                const auto keyValue = static_cast<const unsigned char*>(sqlite3_column_blob(getKeyStmt, 0));
                const auto numBytes = sqlite3_column_bytes(getKeyStmt, 0);
                output.assign(keyValue, keyValue + numBytes);

                CheckSQLite(sqlite3_bind_int64(deleteKeyStmt, 1, link));
                CheckSQLite(sqlite3_bind_int64(deleteKeyStmt, 2, keyId));
                CheckSQLite(sqlite3_step(deleteKeyStmt));
                doCommit = true;
            }
            CheckSQLite(sqlite3_reset(getKeyStmt));
            CheckSQLite(sqlite3_reset(deleteKeyStmt));

            if(doCommit)
            {
                result = SQLiteOk(sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr));
            }
            else
            {
                CheckSQLite(sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr));
            }
            return result;
        } // RemoveKey

        bool FileStore::RemoveKeys(const std::string& destination, Keys& keys)
        {
            // Note: binds are indexed from 1, columns are indexed from 0
            bool result = false;
            const uint64_t link = FNV1aHash(destination);
            bool doCommit = false;

            CheckSQLite(sqlite3_bind_int64(getKeyStmt, 1, link));

            // from docs: After a BEGIN IMMEDIATE, no other database connection will be able to write to the database or do a BEGIN IMMEDIATE or BEGIN EXCLUSIVE. Other processes can continue to read from the database, however.
            CheckSQLite(sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr));
            for(auto& key : keys)
            {
                CheckSQLite(sqlite3_bind_int64(getKeyStmt, 2, key.first));
                if(CheckSQLite(sqlite3_step(getKeyStmt)) == SQLITE_ROW)
                {
                    const auto keyBytes = sqlite3_column_text(getKeyStmt, 0);
                    key.second.assign(keyBytes, keyBytes + sqlite3_column_bytes(getKeyStmt, 0));

                    CheckSQLite(sqlite3_bind_int64(deleteKeyStmt, 1, link));
                    CheckSQLite(sqlite3_bind_int64(deleteKeyStmt, 2, key.first));
                    CheckSQLite(sqlite3_step(deleteKeyStmt));
                    doCommit = true;
                }
                CheckSQLite(sqlite3_reset(getKeyStmt));
                CheckSQLite(sqlite3_reset(deleteKeyStmt));
            }

            if(doCommit)
            {
                CheckSQLite(sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr));
            }
            else
            {
                CheckSQLite(sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr));
            }
            return result;
        } // RemoveKey

        bool FileStore::ReserveKey(const std::string& destination, KeyID& identity)
        {
            // Note: binds are indexed from 1, columns are indexed from 0
            bool result = false;
            const uint64_t link = FNV1aHash(destination);
            // from docs: After a BEGIN IMMEDIATE, no other database connection will be able to write to the database or do a BEGIN IMMEDIATE or BEGIN EXCLUSIVE. Other processes can continue to read from the database, however.
            CheckSQLite(sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr));

            // Find an unused key
            CheckSQLite(sqlite3_bind_int64(getAvailableKeyStmt, 1, link));

            if(CheckSQLite(sqlite3_step(getAvailableKeyStmt)) == SQLITE_ROW)
            {
                // a row exists, use it's key to mark it
                identity = sqlite3_column_int64(getAvailableKeyStmt, 0);
                // set the key as in use
                CheckSQLite(sqlite3_bind_int64(markInUseStmt, 1, link));
                CheckSQLite(sqlite3_bind_int64(markInUseStmt, 2, identity));
                CheckSQLite(sqlite3_step(markInUseStmt));
                CheckSQLite(sqlite3_reset(markInUseStmt));

                CheckSQLite(sqlite3_reset(getAvailableKeyStmt));
                result = SQLiteOk(sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr));

            }
            else
            {
                // no free keys, abort the transaction
                result = false;
                CheckSQLite(sqlite3_reset(getAvailableKeyStmt));
                CheckSQLite(sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr));
            }

            return result;
        } // ReserveKey

        void FileStore::GetCounts(const std::string& destination, uint64_t& availableKeys, uint64_t& remainingCapacity)
        {
            const uint64_t link = FNV1aHash(destination);
            // no storage restrictions
            remainingCapacity = std::numeric_limits<uint64_t>::max();

            // Note: binds are indexed from 1, columns are indexed from 0
            // serialise the calls, the same statement cannot be used by more than thread at a time.
            std::lock_guard<std::mutex> lock(queryMutex);

            CheckSQLite(sqlite3_bind_int64(countKeysStmt, 1, link));
            if(CheckSQLite(sqlite3_step(countKeysStmt)) == SQLITE_ROW)
            {
                availableKeys = sqlite3_column_int64(countKeysStmt, 0);
            }
            CheckSQLite(sqlite3_reset(countKeysStmt));
        }

        uint64_t FileStore::GetNextKeyId(const std::string& destination)
        {
            const uint64_t link = FNV1aHash(destination);
            // Note: binds are indexed from 1, columns are indexed from 0
            uint64_t result = 1;
            // serialise the calls, the same statement cannot be used by more than thread at a time.
            std::lock_guard<std::mutex> lock(queryMutex);

            CheckSQLite(sqlite3_bind_int64(getNextIdStmt, 1, link));
            if(CheckSQLite(sqlite3_step(getNextIdStmt)) == SQLITE_ROW)
            {
                result = sqlite3_column_int64(getNextIdStmt, 0);
            }
            CheckSQLite(sqlite3_reset(getNextIdStmt));
            return result;
        }
    } // namespace keygen
} // namespace cqp
#endif // if defined(SQLITE3_FOUND)
