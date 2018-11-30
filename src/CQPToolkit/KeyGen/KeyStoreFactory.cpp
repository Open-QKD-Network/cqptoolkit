/*!
* @file
* @brief KeyStoreFactory
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/12/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyStoreFactory.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/KeyGen/KeyStore.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Net/DNS.h"
#include "CQPAlgorithms/Util/Strings.h"
#include "CQPToolkit/../Version.h"
#include "CQPToolkit/Util/URI.h"

namespace cqp
{
    namespace keygen
    {
        using grpc::Status;
        using grpc::StatusCode;

        KeyStoreFactory::KeyStoreFactory(std::shared_ptr<grpc::ChannelCredentials> clientCreds, std::shared_ptr<IBackingStore> theBackingStore) :
            clientCreds(clientCreds),
            backingStore(theBackingStore)
        {
        }

        std::string KeyStoreFactory::GetKeystoreName(const std::string& destination)
        {
            using namespace std;
            URI destUri(destination);
            net::IPAddress destIp;
            if(!destUri.ResolveAddress(destIp))
            {
                LOGERROR("Invalid destination address");
            }

            if(destUri.GetPort() == siteAddress.port && destIp != siteAddress.ip)
            {
                // this might be us with a different ip
                std::vector<net::IPAddress> hostIPs = net::GetHostIPs();
                if(std::any_of(hostIPs.begin(), hostIPs.end(), [&destIp](auto myIp) { return myIp == destIp; }))
                {
                    // override the IP to the one we recognise
                    destIp = siteAddress.ip;
                }

            } // if partial match

            const string keystoreName = destIp.ToString() + ":" + to_string(destUri.GetPort());

            return keystoreName;
        }
        std::shared_ptr<KeyStore> KeyStoreFactory::GetKeyStore(const std::string& destination)
        {
            using namespace std;
            std::shared_ptr<KeyStore> result;

            const string keystoreName = GetKeystoreName(destination);
            auto it = keystores.find(keystoreName);

            if(it == keystores.end())
            {
                const string thisSiteName = GetKeystoreName(siteAddress);
                if(thisSiteName != keystoreName)
                {
                    // No keystore exists, create one and return it
                    result.reset(new KeyStore(siteAddress, clientCreds, destination, this, backingStore));
                    for(auto reportCb : reportingCallbacks)
                    {
                        result->stats.Add(reportCb);
                    }
                    keystores[keystoreName] = result;
                }
                else
                {
                    LOGERROR("Refusing to create keystore, destination = this site: " + siteAddress.ToString());
                }
            }
            else
            {
                // return the keystore found
                result = it->second;
            }

            return result;
        } // GetKeyStore

        grpc::Status KeyStoreFactory::GetKeyStores(grpc::ServerContext*, const google::protobuf::Empty*, remote::SiteList* response)
        {
            grpc::Status result;
            for(const auto& ks : keystores)
            {
                response->add_urls(ks.first);
            }
            return result;
        }

        Status KeyStoreFactory::GetSharedKey(grpc::ServerContext*, const cqp::remote::KeyRequest* request, cqp::remote::SharedKey* response)
        {
            using namespace std;
            Status result;
            const string keystoreName = GetKeystoreName(request->siteto());
            auto keyStoreIt = keystores.find(keystoreName);

            if(keyStoreIt == keystores.end())
            {
                result = Status(grpc::StatusCode::INVALID_ARGUMENT, "No key store available for specified sites");
            }
            else
            {
                // there is a keystore
                KeyID keyId = 0;
                PSK keyValue;

                // grpc proto syntax version 3: us oneof to do optional parameters
                if(request->opt_case() == remote::KeyRequest::OptCase::kKeyId)
                {
                    // the request has included a keyid, get an existing key
                    keyId = request->keyid();
                    result = keyStoreIt->second->GetExistingKey(keyId, keyValue);
                }
                else
                {
                    // get an unused key
                    if(!keyStoreIt->second->GetNewKey(keyId, keyValue))
                    {
                        result = Status(StatusCode::RESOURCE_EXHAUSTED, "No key available");
                    }
                } // if keyid specified

                if(result.ok())
                {
                    // store the data in the outgoing variable
                    response->set_keyid(keyId);
                    response->mutable_keyvalue()->assign(keyValue.begin(), keyValue.end());
                    URI pkcs11Url;
                    pkcs11Url.SetScheme("pkcs11");
                    std::vector<std::string> pathElements;
                    pathElements.push_back("type=secret-key");
                    // this = PKCS label, used for the destination - so that it can be searched
                    pathElements.push_back("object=" + URI::Encode(request->siteto()));
                    pathElements.push_back("id=0x" + ToHexString(keyId));
                    pkcs11Url.SetPath(pathElements, ";");

                    // store a url on how to access the key
                    response->set_url(pkcs11Url.ToString());
                    LOGTRACE("Key URL:" + pkcs11Url.ToString());
                }
            } // if keystore found

            return result;
        }

        void KeyStoreFactory::AddReportingCallback(stats::IAllStatsCallback* callback)
        {
            reportingCallbacks.push_back(callback);
            for(const auto& ks : keystores)
            {
                ks.second->stats.Add(callback);
            }
        }

        void KeyStoreFactory::RemoveReportingCallback(stats::IAllStatsCallback* callback)
        {
            for(auto it = reportingCallbacks.begin(); it != reportingCallbacks.end(); it++)
            {
                if(*it == callback)
                {
                    reportingCallbacks.erase(it);
                    break; // for
                }
            }

            for(const auto& ks : keystores)
            {
                ks.second->stats.Remove(callback);
            }
        } // GetSharedKey

        grpc::Status KeyStoreFactory::MarkKeyInUse(grpc::ServerContext*, const remote::KeyRequest* request, remote::KeyIdValue* response)
        {
            grpc::Status result;
            auto keystore = GetKeyStore(request->siteto());
            if(keystore)
            {
                KeyID alternate = 0;
                result = keystore->MarkKeyInUse(request->keyid(), alternate);
                if(result.ok())
                {
                    response->set_keyid(alternate);
                }
            }
            else
            {
                result = Status(StatusCode::INVALID_ARGUMENT, "Unknown keystore path: " + siteAddress.ToString() + " -> " + request->siteto());
            }

            return result;
        } // MarkKeyInUse

        grpc::Status KeyStoreFactory::BuildXorKey(grpc::ServerContext*, const remote::KeyPathRequest* request, google::protobuf::Empty*)
        {
            using namespace std;
            // alias for readability
            const auto& siteList = request->sites().urls();

            PSK finalKey;

            Status result;
            // check the parameters make sense
            if(siteList.size() > 2 && *siteList.rbegin() == siteAddress)
            {
                // Set the initial values for the first run of DoCombinedKey
                KeyID leftKeyID = 0; // don't know this for the first run
                bool leftKeyIDKnown = false;
                KeyID rightKeyID = 0;

                if(!GetKeyStore(*(siteList.rbegin() + 1))->
                        GetNewKey(rightKeyID, finalKey))
                {
                    LOGERROR("Failed to get a new key");
                }


                // loop backwards through each site, skip our site and the first site
                for(auto middleAddress = siteList.rbegin() + 1; middleAddress != siteList.rend() - 1; middleAddress++)
                {
                    std::string combinedKey;
                    // The site before the previous site
                    const auto leftAddress = middleAddress + 1;
                    const auto rightAddress = middleAddress - 1; // starts as our address

                    if(leftAddress == siteList.rend() - 1)
                    {
                        // left address is now alpha, we know this key id
                        leftKeyID = request->originatingkeyid();
                        leftKeyIDKnown = true;
                    }
                    else
                    {
                        leftKeyID = 0;
                        leftKeyIDKnown = false;
                    }

                    result = DoCombinedKey(
                                 *middleAddress,
                                 *leftAddress,
                                 leftKeyID,     /*in out*/
                                 leftKeyIDKnown,
                                 *rightAddress,
                                 rightKeyID,
                                 combinedKey    /*out*/
                             );
                    LOGDEBUG(*leftAddress + "[" + to_string(leftKeyID) + "], " + *rightAddress + "[" + to_string(rightKeyID) + "]" + " = " + to_string(static_cast<unsigned char>(combinedKey[0])));

                    if(result.ok())
                    {
                        // shift the key ids
                        rightKeyID = leftKeyID;

                        // calculate the partial key
                        finalKey ^= combinedKey;
                    }
                    else
                    {
                        LOGERROR("Failed to get combined key");
                        break; // for
                    }
                }

                shared_ptr<keygen::KeyStore> finalKeystore = GetKeyStore(*siteList.begin());
                if(!finalKeystore->StoreReservedKey(request->originatingkeyid(), finalKey))
                {
                    result = Status(StatusCode::ALREADY_EXISTS, "Originating key ID already exists in key store");
                }
            }
            else
            {
                result = Status(StatusCode::INVALID_ARGUMENT, "Invalid path");
            }
            return result;
        } // BuildXorKey

        grpc::Status KeyStoreFactory::DoCombinedKey(
            const std::string& otherSiteAddress,
            const std::string& leftAddress,
            KeyID& leftKeyID,
            bool leftKeyIDKnown,
            const std::string& rightAddress,
            const KeyID& rightKeyID,
            std::string& combinedKey
        )
        {
            using namespace std;
            using remote::IKeyFactory;
            grpc::Status result;
            grpc::ClientContext ctx;
            remote::CombinedKeyRequest combinedKeyRequest;
            remote::CombinedKeyResponse combinedKeyResponse;

            unique_ptr<IKeyFactory::Stub> otherSiteAgent = IKeyFactory::NewStub(GetSiteChannel(otherSiteAddress));

            combinedKeyRequest.set_leftsite(leftAddress);
            if(leftKeyIDKnown)
            {
                combinedKeyRequest.set_leftkeyid(leftKeyID);
            }

            combinedKeyRequest.set_rightsite(rightAddress);
            combinedKeyRequest.set_rightkeyid(rightKeyID);

            result = LogStatus(otherSiteAgent->GetCombinedKey(&ctx, combinedKeyRequest, &combinedKeyResponse));

            if(result.ok())
            {
                if(!leftKeyIDKnown)
                {
                    leftKeyID = combinedKeyResponse.leftid();
                }
                combinedKey = combinedKeyResponse.combinedkey();
            }
            return result;
        }

        grpc::Status KeyStoreFactory::GetCombinedKey(grpc::ServerContext*, const remote::CombinedKeyRequest* request, remote::CombinedKeyResponse* response)
        {
            using namespace std;
            Status result;
            shared_ptr<keygen::KeyStore> leftKeyStore = GetKeyStore(request->leftsite());
            shared_ptr<keygen::KeyStore> rightKeyStore = GetKeyStore(request->rightsite());

            PSK leftKey;
            PSK rightKey;

            if(request->leftKey_case() == remote::CombinedKeyRequest::LeftKeyCase::kLeftKeyId)
            {
                result = leftKeyStore->GetExistingKey(request->leftkeyid(), leftKey);
                if(result.ok())
                {
                    response->set_leftid(request->leftkeyid());
                    LOGDEBUG("Left Key id=" + to_string(request->leftkeyid()) + " value=" + to_string(leftKey[0]));
                }
            }
            else
            {
                KeyID leftKeyId = 0;
                if(leftKeyStore->GetNewKey(leftKeyId, leftKey))
                {
                    LOGDEBUG("Left Key id=" + to_string(leftKeyId) + " value=" + to_string(leftKey[0]));
                    response->set_leftid(leftKeyId);
                }
                else
                {
                    result = Status(StatusCode::RESOURCE_EXHAUSTED, "Failed to get left key");
                }
            }

            result = rightKeyStore->GetExistingKey(request->rightkeyid(), rightKey);

            if(result.ok())
            {
                leftKey ^= rightKey;
                response->mutable_combinedkey()->assign(
                    leftKey.begin(), leftKey.end());
            }

            return result;
        } // GetCombinedKey

        std::shared_ptr<grpc::Channel> KeyStoreFactory::GetSiteChannel(const std::string& connectionAddress)
        {
            using namespace grpc;
            std::shared_ptr<grpc::Channel> result = otherSites[connectionAddress];

            if(result == nullptr)
            {
                // unknown address, create a new channel
                result = CreateChannel(connectionAddress, clientCreds);
                otherSites[connectionAddress] = result;
            }

            return result;
        }
    } // namespace keygen
} // namespace cqp

