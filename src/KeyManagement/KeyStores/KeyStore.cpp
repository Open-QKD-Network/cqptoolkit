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
#include "KeyStore.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/KeyGen/Stats.h"
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include <numeric>

namespace cqp
{
    namespace keygen
    {
        using grpc::Status;
        using grpc::StatusCode;
        using google::protobuf::Empty;
        using grpc::ClientContext;

        KeyStore::KeyStore(const std::string& thisSiteAddress, std::shared_ptr<grpc::ChannelCredentials> creds, const std::string& destination,
                           KeyStoreFactory* ksf, std::shared_ptr<IBackingStore> bs, uint64_t cacheLimit) :
            mySiteFrom(thisSiteAddress),
            keystoreFactory(ksf),
            backingStore(bs),
            cacheThreashold(cacheLimit)
        {
            LOGDEBUG("New key store from " + thisSiteAddress + " to " + destination);
            mySiteTo = destination;
            if(backingStore)
            {
                // sync our key is with the backing store so we don't clash with existing keys.
                nextKeyId = backingStore->GetNextKeyId(mySiteTo);
            }

            /// channel to the paired site agent
            std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(mySiteTo, creds);
            if(channel)
            {
                // create a connection to the other site agent
                partnerFactory = remote::IKeyFactory::NewStub(channel);
            }
            else
            {
                LOGERROR("Failed to connect to other site agent");
            }
        }

        KeyStore::~KeyStore()
        {
            FlushCache();
            shutdown = true;
            allKeys_cv.notify_all();

        } // KeyStore

        grpc::Status KeyStore::GetExistingKey(const KeyID& identity, PSK& output)
        {
            LOGTRACE("ID:" + std::to_string(identity));
            grpc::Status result = Status(StatusCode::NOT_FOUND, "No key found within timeout.");

            std::unique_lock<std::mutex> lock(allKeys_lock);
            bool waitResult = allKeys_cv.wait_for(lock, waitTimeout, [&]
            {
                bool result = shutdown;
                if(!shutdown)
                {
                    auto internal = reservedKeys.find(identity);

                    if(internal != reservedKeys.end())
                    {
                        output = internal->second;
                        reservedKeys.erase(internal);
                        result = true;
                    }
                    else if(backingStore != nullptr)
                    {
                        // try the backing store
                        result = backingStore->RemoveKey(mySiteTo, identity, output);
                    }
                }
                return result;
            });

            if(waitResult)
            {
                result = Status();
            }

            return result;
        } // GetExistingKey

        bool KeyStore::GetNewKey(KeyID& identity, PSK& output, bool waitForKey)
        {
            LOGTRACE("");
            // see if we've already got some key
            // if there's no path, wait until key arrives
            bool result = GetNewDirectKey(identity, output, myPath.empty() && waitForKey);

            if(!result && !myPath.empty())
            {
                // no locally defined keys
                // build key from path
                result = GetNewIndirectKey(identity, output);
            }
            LOGTRACE("ID:" + std::to_string(identity));
            return result;
        }

        bool KeyStore::ReserveNewKey(std::unique_lock<std::mutex>&, KeyID& keyID)
        {
            LOGTRACE("");
            bool result = !unusedKeys.empty();
            if(result)
            {
                KeyMap::iterator keyFound = unusedKeys.begin();
                keyID = keyFound->first;
                reservedKeys[keyFound->first] = keyFound->second;

                // remove the key from the list
                unusedKeys.erase(keyFound);
            }
            else if(backingStore != nullptr)
            {
                result = backingStore->ReserveKey(mySiteTo, keyID);
                if(result)
                {
                    result = backingStore->RemoveKey(mySiteTo, keyID, reservedKeys[keyID]);
                }
            }
            return result;
        }

        void KeyStore::FlushCache()
        {
            if(backingStore)
            {
                IBackingStore::Keys backingStoreKeys;

                /*lock scope*/
                {
                    std::unique_lock<std::mutex> lock(allKeys_lock);

                    // move the keys out to the backing store
                    for(KeyMap* list :
                            {
                                &unusedKeys, &reservedKeys
                            })
                    {
                        std::copy(list->begin(), list->end(), std::back_inserter(backingStoreKeys));

                        list->clear();
                    } // for list
                } /*lock scope*/

                if(!backingStore->StoreKeys(mySiteTo, backingStoreKeys))
                {
                    LOGERROR("Failed to move keys to backing store");
                }
            } // if backing store
        } // FlushCache

        bool KeyStore::GetNewDirectKey(KeyID& identity, PSK& output, bool waitForKey)
        {
            LOGTRACE("");
            bool result = false;
            KeyID keyID = 0;

            if(partnerFactory == nullptr)
            {
                LOGERROR("Keystore not connected.");
                return false;
            }

            /*lock scope*/
            {
                std::unique_lock<std::mutex> lock(allKeys_lock);

                if(waitForKey)
                {
                    result = allKeys_cv.wait_for(lock, waitTimeout, [&]
                    {
                        return shutdown || ReserveNewKey(lock, keyID);
                    });
                }
                else
                {
                    // fail immediately if there's no key available
                    result = ReserveNewKey(lock, keyID);
                }

                if(result)
                {
                    stats.keyUsed.Update(1);
                }
            }/*lock scope*/

            if(result)
            {
                ClientContext ctx;
                remote::KeyRequest request;
                request.set_siteto(mySiteFrom);
                request.set_keyid(keyID);
                remote::KeyIdValue response;
                // call the other site and make sure the key isn't used for anything else
                Status markResult = LogStatus(
                                        partnerFactory->MarkKeyInUse(&ctx, request, &response));

                if (markResult.ok())
                {
                    if(response.keyid() == keyID)
                    {
                        LOGDEBUG("Reserved original key " + std::to_string(keyID));
                        // our key has been reserved on the other side
                        std::lock_guard<std::mutex> lock(allKeys_lock);

                        identity = keyID;
                        output = reservedKeys[keyID];
                        reservedKeys.erase(keyID);
                        result = true;
                    } // if keyids match
                    else
                    {
                        LOGDEBUG("Reserved alternate key " + std::to_string(response.keyid()));
                        // the key is already in use but an alternative has been supplied

                        std::unique_lock<std::mutex> lock(allKeys_lock);
                        bool result = allKeys_cv.wait_for(lock, waitTimeout, [&]
                        {
                            bool result = shutdown;
                            if(!shutdown)
                            {
                                KeyMap::iterator keyFound = unusedKeys.find(response.keyid());
                                KeyMap::iterator keyFoundReserved = reservedKeys.find(response.keyid());
                                if(keyFound != unusedKeys.end())
                                {
                                    // The key has arrived on our side so use it.
                                    identity = keyFound->first;
                                    output = keyFound->second;
                                    // remove the key from the list
                                    unusedKeys.erase(keyFound);
                                    result = true;
                                }
                                else if(keyFoundReserved != reservedKeys.end())
                                {
                                    // The key has arrived on our side so use it.
                                    identity = keyFoundReserved->first;
                                    output = keyFoundReserved->second;
                                    // remove the key from the list
                                    reservedKeys.erase(keyFoundReserved);
                                    result = true;
                                }
                                else if(backingStore != nullptr)
                                {
                                    result = backingStore->RemoveKey(mySiteTo, response.keyid(), output);
                                }
                            }
                            return result;
                        });

                        if(!result)
                        {
                            LOGERROR("Failed to find unused key. Please retry.");
                        } // else key found
                    } // else keyids match
                } // if result ok
                else
                {
                    // something went very wrong
                    LOGERROR("Key allocation failed.");
                    result = false;
                } // else result ok
            } // if result
            return result;
        } // GetNewKey

        bool KeyStore::GetNewIndirectKey(KeyID& identity, PSK& output)
        {
            LOGTRACE("");
            bool result = true;

            remote::KeyPathRequest request;

            (*request.mutable_sites()->mutable_urls()->Add()) = mySiteFrom;
            for(const auto& element : myPath)
            {
                (*request.mutable_sites()->mutable_urls()->Add()) = element;
            }
            (*request.mutable_sites()->mutable_urls()->Add()) = mySiteTo;

            const std::string& nextHopAddress = *(request.sites().urls().begin() + 1);

            auto hopKeyStore = keystoreFactory->GetKeyStore(nextHopAddress);
            result = hopKeyStore->GetNewKey(identity, output, true);
            if(result)
            {
                LOGDEBUG("First hop key: id=" + std::to_string(identity) + " value=" + std::to_string(output[0]));
            }
            else
            {
                LOGERROR("Failed to get a key from the next hop: " + nextHopAddress);
            }

            if(result)
            {
                grpc::ClientContext ctx;
                Empty response;
                request.set_originatingkeyid(identity);

                // TODO: This could be wrapped in async and the call made asynchronous so that the
                // building can go on while this returns to the caller.
                Status buildStatus = LogStatus(partnerFactory->BuildXorKey(&ctx, request, &response));
                result = buildStatus.ok();

                stats.keyUsed.Update(1);
                stats.keyGenerated.Update(1);
                stats.unusedKeysAvailable.Update(unusedKeys.size());
                stats.reservedKeys.Update(reservedKeys.size());
            }

            return result;
        } // GetNewIndirectKey

        grpc::Status KeyStore::MarkKeyInUse(const KeyID& identity, KeyID& alternative)
        {
            LOGTRACE("");
            grpc::Status result;
            bool reserveAlternative = false;

            std::unique_lock<std::mutex> lock(allKeys_lock);

            auto reservedIt = reservedKeys.find(identity);
            auto unusedIt = unusedKeys.find(identity);
            if(reservedIt != reservedKeys.end())
            {
                // The key has been reserved by our GetNewKey
                // reserve an alternative
                reserveAlternative = true;
            }
            else if(unusedIt != unusedKeys.end())
            {
                // good to go, reserve the key
                reservedKeys[unusedIt->first] = unusedIt->second;
                alternative = unusedIt->first;
                unusedKeys.erase(unusedIt);
            }
            // try getting it from the backing store
            else if(backingStore && backingStore->RemoveKey(mySiteTo, identity, reservedKeys[identity]))
            {
                // key was extracted from the backing store
                alternative = identity;
            }
            else // if(reservedIt == reservedKeys.end())
            {
                // key hasn't arrived yet or it's already taken
                // add a placeholder to make sure the key isn't used elsewhere
                reservedKeys[identity] = PSK();
                // wait for key to arrive

                bool keyFound = allKeys_cv.wait_for(lock, waitTimeout, [&]
                {
                    // now there is a placeholder in reserved, it wont be sent to the backing store
                    reservedIt = reservedKeys.find(identity);
                    return shutdown || reservedIt != reservedKeys.end();
                });

                if(keyFound)
                {
                    // good to go, the key has been reserved
                    alternative = identity;
                }
                else
                {
                    // timeout, looks like the key was already taken
                    reserveAlternative = true;
                    // remove the stale reservation
                    reservedKeys.erase(identity);
                }
            }

            if(reserveAlternative)
            {
                bool keyFound = allKeys_cv.wait_for(lock, waitTimeout, [&]
                {
                    // this will move the key to the reserved list
                    return shutdown || ReserveNewKey(lock, alternative);
                });

                if(keyFound)
                {
                    result = Status(StatusCode::OK, "Key already reserved, alternative supplied.");
                }
                else
                {
                    result = Status(StatusCode::UNAVAILABLE, "Key already reserved, no new keys available.");
                }
            } // reserveAlternative

            return result;
        }

        bool KeyStore::StoreReservedKey(const KeyID& id, const PSK& keyValue)
        {
            bool result = false;
            std::lock_guard<std::mutex> lock(allKeys_lock);
            if(unusedKeys.find(id) == unusedKeys.end())
            {
                reservedKeys[id] = keyValue;
                result = true;
                stats.reservedKeys.Update(reservedKeys.size());
            }
            return result;
        }

        void KeyStore::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
        {
            LOGTRACE(mySiteFrom + " to " + mySiteTo + " receiving " + std::to_string(keyData->size()) + " key(s)");
            IBackingStore::Keys backingStoreKeys;

            /*lock scope*/
            {
                std::lock_guard<std::mutex> lock(allKeys_lock);

                for(PSK key : *keyData)
                {
                    KeyID nextId = nextKeyId.fetch_add(1);
                    if(reservedKeys.find(nextId) != reservedKeys.end())
                    {
                        // key has already been marked as reserved
                        reservedKeys[nextId] = key;
                    }
                    else if(unusedKeys.find(nextId) != unusedKeys.end())
                    {
                        LOGERROR("KeyID already in use:" + std::to_string(nextId));
                    }
                    else if(unusedKeys.size() >= cacheThreashold && backingStore)
                    {
                        backingStoreKeys.push_back({nextId, key});
                    }
                    else
                    {
                        unusedKeys[nextId] = key;
                    }

                } // for keyData
            }/*lock scope*/

            if(!backingStoreKeys.empty())
            {
                if(!backingStore->StoreKeys(mySiteTo, backingStoreKeys))
                {
                    LOGWARN("Failed to send keys to backing store, storing locally");
                    for(const auto& key : backingStoreKeys)
                    {
                        unusedKeys[key.first] = key.second;
                    }
                }
            }
            // we've changed the list so notify any waiting threads
            allKeys_cv.notify_all();

            uint64_t unusedAvailable = 0;
            if(backingStore)
            {
                uint64_t remainingCapacity = 0;
                backingStore->GetCounts(mySiteTo, unusedAvailable, remainingCapacity);
            }
            unusedAvailable += unusedKeys.size();
            // publish some stats
            stats.keyGenerated.Update(keyData->size());
            stats.unusedKeysAvailable.Update(unusedAvailable);
            stats.reservedKeys.Update(reservedKeys.size());
        }

        bool KeyStore::SetPath(const std::vector<std::string>& path)
        {
            bool result = false;
            /* lock scope */
            {
                // make sure we're not in the middle of key generation
                std::lock_guard<std::mutex> lock(allKeys_lock);
                myPath = path;
                result = true;
            }/* lock scope */

            std::string msg = "Path is now " + mySiteFrom + " -> ";
            for(const auto& hop : path)
            {
                msg += hop + " -> ";
            }
            msg += mySiteTo;
            LOGDEBUG(msg);
            return result;
        }
    } // namespace keygen
} // namespace cqp
