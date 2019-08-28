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
#include "KeyManagement/KeyStores/IKeyStore.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "QKDInterfaces/IKeyFactory.grpc.pb.h"
#include "KeyManagement/KeyStores/IBackingStore.h"
#include "CQPToolkit/KeyGen/Stats.h"
#include <map>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace cqp
{
    namespace keygen
    {

        class KeyStoreFactory;

        /// @brief Stores and manages pre shared key
        class KEYMANAGEMENT_EXPORT KeyStore : public virtual IKeyStore, public virtual IKeyCallback
        {
        public:
            /// Default constructor
            /// @param thisSiteAddress The site which this is running on
            /// @param creds Credentials used to connect to the peer
            /// @param destination The other side which will have the matching keys
            /// @param ksf The key store factory to get other key stores from
            /// @param bs how to archive keys, nullptr = disabled
            KeyStore(const std::string& thisSiteAddress, std::shared_ptr<grpc::ChannelCredentials> creds,
                     const std::string& destination, KeyStoreFactory* ksf = nullptr, std::shared_ptr<IBackingStore> bs = nullptr,
                     uint64_t cacheLimit = 100000);

            /// Destructor
            ~KeyStore() override;

            /// @name IKeyStore Interface realisation
            ///@{
            /// @copydoc IKeyStore::GetExistingKey
            grpc::Status GetExistingKey(const KeyID& identity, PSK& output) override;

            /// @copydoc IKeyStore::GetNewKey
            bool GetNewKey(KeyID& identity, PSK& output, bool waitForKey = false) override;

            /// @copydoc IKeyStore::MarkKeyInUse
            grpc::Status MarkKeyInUse(const KeyID& identity, KeyID& alternative) override;

            /// @copydoc IKeyStore::StoreReservedKey
            bool StoreReservedKey(const KeyID& id, const PSK& keyValue) override;

            /// @copydoc IKeyStore::SetPath
            bool SetPath(const std::vector<std::string>& path) override;

            ///@}

            /// @name IKeyCallback Interface realisation
            ///@{

            /** @copydoc IKeyCallback::OnKeyGeneration
             * @details
             * @startuml KeyStoreIncomingKey
             *     title KeyStore Incoming Key
             *     hide footbox
             *
             *     participant KeyStore as ks
             *     database reservedKeys as rk
             *     database cache
             *     boundary IBackingStore as bs
             *
             *     [-> ks : OnKeyGeneration()
             *     activate ks
             *     alt Key is reserved
             *         note over ks
             *             Store key in reserved list
             *         end note
             *         ks -> rk : Push(key)
             *     else if too many cached keys
             *         ks -> bs : StoreKeys(key)
             *     else
             *         note over ks
             *             Add to local key cache
             *         end note
             *         ks -> cache : Push(key)
             *     end alt
             *     deactivate ks
             * @enduml
             */
            virtual void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;

            ///@}

            /**
             * @brief GetNumberUnusedKeys
             * @return number of unused keys
             */
            size_t GetNumberUnusedKeys() const
            {
                return unusedKeys.size();
            }

            /**
             * @brief ReserveNewKey
             * Move an unused key into the reserved list and return it's id
             * @param allKeysLock
             * @param keyID
             * @return True if a key was reserved
             */
            bool ReserveNewKey(std::unique_lock<std::mutex>& allKeysLock, KeyID& keyID);

            /**
             * @brief FlushCache
             * Send any unused key to the backing store
             * If there is no backing store this has no effect.
             */
            void FlushCache();

            /**
             * @brief SetCacheThreashold
             * Sets the number of keys to hold in memory.
             * Once this limit is reached, any new keys will be sent to the backing store.
             * Lowering this value will not immediately move key to the backing store.
             * @param limit The new cache limit
             */
            void SetCacheThreashold(uint64_t limit)
            {
                cacheThreashold = limit;
            }

            /// stats collected by this class
            Statistics stats;
        protected:// members
            /// When trying to find a key, the operation will fail after this timeout
            std::chrono::milliseconds waitTimeout = std::chrono::seconds(30);
            /// Stores both key value and authentication requirements
            struct AuthPSK
            {
                /// The key value
                PSK psk;
                /// The authentication token which this key belongs to.
                std::string authToken;
            };
            /// Maps key ids to the key data
            using KeyMap = std::map<KeyID, PSK>;
            /// Stores all currently available keys
            KeyMap unusedKeys;
            /// Stores keys which can only be retrieved by id
            KeyMap reservedKeys;
            /// Protects access to allKeys
            std::mutex allKeys_lock;
            /// variable to wait on for changes
            std::condition_variable allKeys_cv;
            /// The keystore factory at the other site
            std::unique_ptr<remote::IKeyFactory::Stub> partnerFactory;
            /// Endpoint which this keystore is holding keys for
            const std::string mySiteFrom;
            /// Endpoint which this keystore is holding keys for
            std::string mySiteTo;
            /// The hops needed to create a key
            /// @details 2 hops = direct key exchange
            std::vector<std::string> myPath;
            /// The keys store where we can get direct key stores from
            KeyStoreFactory* keystoreFactory = nullptr;
            /// where the keys are archived to
            std::shared_ptr<IBackingStore> backingStore;
            /// How many keys to store locally before sending them to the backing store
            uint64_t cacheThreashold;
            /// a counter for assigning incoming keys an id
            std::atomic_uint64_t nextKeyId {1};
            /// for stopping internal threads
            std::atomic_bool shutdown { false};
        protected: // methods

            /**
             * @brief GetNewDirectKey
             * Allocate a key from existing keys
             * @param[out] identity The key id allocated
             * @param[out] output The key value
             * @param[in] waitForKey If false, the function will return immediately if there is no key available
             * @return true if a key was successfully allocated
             */
            bool GetNewDirectKey(KeyID& identity, PSK& output, bool waitForKey);

            /**
             * @brief GetNewIndirectKey
             * Create a key from a chain of direct stores
             * @param[out] identity The key id allocated
             * @param[out] output The key value
             * @return true if a key was successfully created
             */
            bool GetNewIndirectKey(KeyID& identity, PSK& output);
        };

    } // namespace keygen
} // namespace cqp
