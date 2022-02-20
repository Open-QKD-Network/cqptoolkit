/*!
* @file
* @brief SiteAgent
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteAgent.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Statistics/StatisticsLogger.h"


#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "KeyManagement/KeyStores/HSMStore.h"
#include "KeyManagement/KeyStores/BackingStoreFactory.h"

#include "KeyManagement/Net/ServiceDiscovery.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "Algorithms/Net/DNS.h"
#include "Algorithms/Util/Env.h"
#include "Algorithms/Util/Threading.h"

#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "QKDInterfaces/Device.pb.h"
#include "CQPToolkit/Auth/AuthUtil.h"

#include <google/protobuf/util/message_differencer.h>
#include <unordered_map>

#include "KeyManagement/SDN/NetworkManager.h"

#if defined(SQLITE3_FOUND)
    #include "KeyManagement/KeyStores/FileStore.h"
#endif
namespace cqp
{

    using grpc::Status;
    using grpc::StatusCode;

    void SiteAgent::RegisterWithNetMan(std::string netManUri, std::shared_ptr<grpc::ChannelCredentials> creds)
    {
        using namespace std;

        LOGINFO("Connecting to Network Manager: " + netManUri);
        /// channel to the network manager
        std::shared_ptr<grpc::Channel> netmanChannel = grpc::CreateChannel(netManUri, creds);
        /// The network manager to register with
        std::unique_ptr<remote::INetworkManager::Stub> netMan = remote::INetworkManager::NewStub(netmanChannel);

        if(server && netMan)
        {
            // keep a copy of what was sent last time
            remote::Site siteDetailsCopy;

            grpc::Status registerResult = Status(StatusCode::INTERNAL, "");
            do
            {
                google::protobuf::Empty response;
                grpc::ClientContext ctx;

                {
                    /*lock scope*/
                    unique_lock<mutex> lock(siteDetailsMutex);
                    if(registerResult.ok())
                    {
                        // we've registered - wait for any change in the details
                        siteDetailsChanged.wait(lock, [&]()
                        {
                            google::protobuf::util::MessageDifferencer diff;
                            return !diff.Equals(siteDetails, siteDetailsCopy) || shutdown;
                        });
                    }
                    siteDetailsCopy = siteDetails;

                }/*lock scope*/

                if(shutdown)
                {
                    remote::SiteAddress siteAddress;
                    (*siteAddress.mutable_url()) = siteDetails.url();
                    // register if the last attempt failed or the site details have changed
                    registerResult = LogStatus(
                                         netMan->UnregisterSite(&ctx, siteAddress, &response), "Failed to register site with Network Manager");

                    if(!registerResult.ok())
                    {
                        netmanChannel->WaitForConnected(chrono::system_clock::now() +
                                                        chrono::seconds(3));
                    }
                }
                else
                {
                    // register if the last attempt failed or the site details have changed
                    registerResult = LogStatus(
                                         netMan->RegisterSite(&ctx, siteDetailsCopy, &response), "Failed to register site with Network Manager");
                }

            }
            while(!shutdown);
        }
        else
        {
            LOGERROR("Invalid setup");
        }
    }

    SiteAgent::SiteAgent(const remote::SiteAgentConfig& config) :
        myConfig(config)
    {
        if(myConfig.id().empty() || !UUID::IsValid(myConfig.id()))
        {
            myConfig.set_id(UUID());
            LOGINFO("Invalid ID. Setting to " + myConfig.id());
        }

        keystoreFactory.reset(new keygen::KeyStoreFactory(LoadChannelCredentials(config.credentials()),
                              keygen::BackingStoreFactory::CreateBackingStore(myConfig.backingstoreurl())));
        reportServer.reset(new stats::ReportServer());
        reportServer->AddAdditionalProperties("siteName", config.name());

        // attach the reporting to the device factory so it link them when creating devices
        keystoreFactory->AddReportingCallback(reportServer.get());
        // for debug
        //deviceFactory->AddReportingCallback(statsLogger.get());

        if(config.limitCache_case() == remote::SiteAgentConfig::LimitCacheCase::kKeyStoreCache)
        {
            keystoreFactory->SetKeyStoreCacheLimit(config.keystorecache());
        }

        // create the server
        grpc::ServerBuilder builder;

        // grpc will create worker threads as it needs, idle work threads
        // will be stopped if there are more than this number running
        // setting this too low causes large number of thread creation+deletions, default = 2
        builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 50);

        int listenPort = static_cast<int>(myConfig.listenport());

        if(myConfig.bindaddress().empty())
        {
            builder.AddListeningPort(std::string(net::AnyAddress) + ":" + std::to_string(myConfig.listenport()),
                                     LoadServerCredentials(config.credentials()), &listenPort);
        }
        else
        {
            builder.AddListeningPort(myConfig.bindaddress() + ":" + std::to_string(myConfig.listenport()),
                                     LoadServerCredentials(config.credentials()), &listenPort);
        }

        // Use the internal network manager if there are static links to manage
        if(!config.statichops().empty())
        {
            LOGINFO("Creating an internal network manager for " + std::to_string(config.statichops().size()) + " static links");
            internalNetMan = std::make_unique<NetworkManager>(config.statichops(), LoadChannelCredentials(config.credentials()));
            builder.RegisterService(internalNetMan.get());
        }

        // Register services
        builder.RegisterService(this);
        builder.RegisterService(static_cast<remote::IKeyFactory::Service*>(keystoreFactory.get()));
        builder.RegisterService(static_cast<remote::IKey::Service*>(keystoreFactory.get()));
        builder.RegisterService(reportServer.get());
        // ^^^ Add new services here ^^^ //

        server = builder.BuildAndStart();
        if(!server)
        {
            LOGERROR("Failed to create server");
        }
        myConfig.set_listenport(static_cast<uint32_t>(listenPort));

        if(myConfig.connectionaddress().empty())
        {
            myConfig.set_connectionaddress(net::GetHostname(true) + ":" + std::to_string(myConfig.listenport()));
        } else {
            myConfig.set_connectionaddress(myConfig.connectionaddress() + ":" + std::to_string(myConfig.listenport()));
        }

        LOGINFO("My address is: " + myConfig.connectionaddress());
        reportServer->AddAdditionalProperties("siteFrom", myConfig.connectionaddress());

        (*siteDetails.mutable_url()) = myConfig.connectionaddress();
        // tell the key store factory our address now we have it
        keystoreFactory->SetSiteAddress(GetConnectionAddress());

        if(!config.netmanuri().empty())
        {
            netManRegister = std::thread(&SiteAgent::RegisterWithNetMan, this, config.netmanuri(), LoadChannelCredentials(config.credentials()));
        }

        // register with the internal network manager
        if(internalNetMan)
        {
            LOGINFO("Registering with internal network manager");
            LogStatus(internalNetMan->RegisterSite(nullptr, &siteDetails, nullptr));
        }
    } // SiteAgent

    SiteAgent::~SiteAgent()
    {
        shutdown = true;
        // trigger the unregistering from the network manager
        siteDetailsChanged.notify_all();

        // disconnect all session controllers
        for(auto device : devicesInUse)
        {
            device.second->Stop();
        }
        devicesInUse.clear();

        if(netManRegister.joinable())
        {
            netManRegister.join();
        }

        // register with the internal network manager
        if(internalNetMan)
        {
            LOGINFO("Unregistering from internal network manager");
            remote::SiteAddress request;
            request.set_url(siteDetails.url());
            LogStatus(internalNetMan->UnregisterSite(nullptr, &request, nullptr));
        }

        server->Shutdown();

        if(reportServer)
        {
            if(keystoreFactory)
            {
                keystoreFactory->RemoveReportingCallback(reportServer.get());
            }
        }

    } // ~SiteAgent

    bool SiteAgent::RegisterWithDiscovery(net::ServiceDiscovery& sd)
    {
        bool result = true;

        RemoteHost sdhost;
        // generate a unique name by appending the port number
        // this could be anything
        if(myConfig.name().empty())
        {
            sdhost.name = "SiteAgent-" + std::to_string(myConfig.listenport());
        }
        else
        {
            sdhost.name = myConfig.name();
        }
        sdhost.id = myConfig.id();
        sdhost.port = static_cast<uint16_t>(myConfig.listenport());
        // TODO: get these automatically
        sdhost.interfaces.insert(remote::ISiteAgent::service_full_name());
        sdhost.interfaces.insert(remote::IKeyFactory::service_full_name());
        sdhost.interfaces.insert(remote::IKey::service_full_name());
        sdhost.interfaces.insert(remote::IReporting::service_full_name());
        // ^^^ Add new services here ^^^ //
        sd.SetServices(sdhost);
        return result;
    } // RegisterWithDiscovery

    void SiteAgent::ProcessKeys(const std::string& deviceId, std::shared_ptr<DeviceConnection> connection, std::unique_ptr<PSK> initialKey)
    {
        using namespace std;
        //Device Stub Setup
        auto deviceStub = remote::IDevice::NewStub(connection->channel);

        //Key Transfer Stub Setup
        //TODO get these automatically
        string keyTransferHost = "localhost";
        string keyTransferPort = "50051";
        int keysSent = 1;

        int qllBlockSz = 4096;
        int maxKeyBlocks = 10;

        auto keyTransferChannel = grpc::CreateChannel(keyTransferHost + ":" + keyTransferPort, grpc::InsecureChannelCredentials());
        auto keyTransferStub = cqp::remote::KeyTransfer::NewStub(keyTransferChannel);
        
        //Key Processing
        if(deviceStub && connection->keySink)
        {
            remote::LocalSettings request;
            remote::RawKeys incommingKeys;
            request.set_initialkey(initialKey->data(), initialKey->size());

            auto reader = deviceStub->WaitForSession(&connection->keyReaderContext, request);

            // TODO this isn't enough to ensure the memory is clean.
            request.clear_initialkey();
            initialKey->clear();
            initialKey.reset();

            while(reader && reader->Read(&incommingKeys) && keysSent < qllBlockSz * maxKeyBlocks)
            {
                auto keys = make_unique<KeyList>();
                keys->reserve(static_cast<size_t>(incommingKeys.keydata().size()));

                // copy each key to the internal datatype
                for(const auto& newKey : incommingKeys.keydata())
                {
                    // get an iterator in the list of keys
                    auto dest = keys->emplace(keys->end());
                    // make space for the key bytes
                    dest->resize(newKey.size());
                    // copy the key bytes into the new storage
                    copy(newKey.begin(), newKey.end(), dest->begin());

                    //Key Transfer to Open-QKD-Network
                    cqp::remote::Key keyMessage;
                    keyMessage.set_key(newKey);
                    keyMessage.set_seqid(keysSent);
                    keyMessage.set_localid(deviceId);
                    keysSent +=1;

                    grpc::ClientContext context;
                    cqp::remote::Empty response;

                    grpc::Status status = keyTransferStub->OnKeyFromCQP(&context, keyMessage, &response);

                    while (!status.ok()) {
                        LOGINFO("STATUS ERROR: error_code=" + to_string(status.error_code()) + ",msg=" + status.error_message());
                        grpc::ClientContext context;
                        cqp::remote::Empty response;
                        status = keyTransferStub->OnKeyFromCQP(&context, keyMessage, &response);
                    }
                }
                // send the keys on
                connection->keySink->OnKeyGeneration(move(keys));
            }

            reader->Finish();

        }
        else
        {
            LOGERROR("Faild to WaitForSession, invalid setup");
        }
    }

    const remote::ControlDetails* SiteAgent::FindDeviceDetails(const remote::Site& siteDetails, const std::string& deviceId)
    {
        const remote::ControlDetails* result = nullptr;

        for(const auto& dev : siteDetails.devices())
        {
            if(dev.config().id() == deviceId)
            {
                result = &dev;
                break; // for
            }
        }
        return result;
    }

    grpc::Status SiteAgent::PrepHop(const std::string& deviceId, const std::string& destination, remote::SessionDetails& params)
    {
        using namespace std;
        using namespace grpc;
        Status result;
        LOGTRACE("From " + GetConnectionAddress() + " to " + destination + " with device " + deviceId);

        // get the device we've been told to use
        auto localDev = devicesInUse.find(deviceId);
        if(localDev == devicesInUse.end())
        {
            result = Status(StatusCode::NOT_FOUND, "Device " + deviceId + " not found");
            lock_guard<mutex> lock(siteDetailsMutex);
            // not currently in use, find it in the registered devices list
            for(const auto& regDevice : siteDetails.devices())
            {
                if(regDevice.config().id() == deviceId)
                {
                    // store the new device and get the iterator to it for later
                    auto localDev = make_shared<DeviceConnection>();
                    LOGTRACE("Connecting to device control at " + regDevice.controladdress());
                    localDev->channel = grpc::CreateChannel(regDevice.controladdress(), LoadChannelCredentials(myConfig.credentials()));
                    localDev->keySink = keystoreFactory->GetKeyStore(destination);
                    if(localDev->keySink)
                    {
                        if(localDev->channel->WaitForConnected(chrono::system_clock::now() + chrono::seconds(10)))
                        {
                            result = Status(StatusCode::UNAVAILABLE, "Failed to connect to " + regDevice.controladdress());
                        }

                        auto initialPsk = std::make_unique<PSK>();

                        // find a key to use for bootstrapping
                        // First try our own key store.
                        if(params.initialkeyid() == 0)
                        {
                            KeyID newId;
                            if(localDev->keySink->GetNewKey(newId, *initialPsk))
                            {
                                // set the key ID so that it is passed to the other site
                                params.set_initialkeyid(newId);
                            }
                            else
                            {
                                // If theres no key to hand, try the fallback
                                if(!myConfig.fallbackkey().empty())
                                {
                                    LOGWARN("Using fallback key to bootstrap device");
                                    initialPsk->resize(myConfig.fallbackkey().size());
                                    initialPsk->assign(myConfig.fallbackkey().begin(), myConfig.fallbackkey().end());
                                }
                                else
                                {
                                    LOGWARN("No key available for bootstrap. Ether populate the keystores or provide a fallback key in the configuration.");
                                }
                            }
                        }
                        else
                        {
// we should be able to get the key from our local key store
                            if(!LogStatus(localDev->keySink->GetExistingKey(params.initialkeyid(), *initialPsk)).ok())
                            {
                                LOGERROR("Failded to get existing key " + std::to_string(params.initialkeyid()));
                            }
                        }

                        LOGDEBUG("Starts key reader thread...");
                        localDev->readerThread = thread(&SiteAgent::ProcessKeys, deviceId, localDev, move(initialPsk));

                        // read stats and pass them on
                        localDev->statsThread = thread(&SiteAgent::DeviceConnection::ReadStats, localDev.get(), reportServer.get(), destination);
                        // make the stats thread lower priority than the key reader
                        threads::SetPriority(localDev->statsThread, 1);

                        devicesInUse.emplace(deviceId, move(localDev));
                        // The session will be start by the right side as it has all the required details
                        result = Status();
                        break; // for
                    } // if keystore valid
                    else
                    {
                        result = Status(StatusCode::NOT_FOUND, "Invalid local keystore");
                    }
                } // if device id match
            } // for each siteDetails.devices
        }
        else
        {
            LOGTRACE("Hop already active");
        }

        return result;
    } // PrepHop

    grpc::Status SiteAgent::ForwardOnStartNode(const remote::PhysicalPath* path, const std::string& secondSite)
    {
        using namespace std;
        using namespace grpc;
        grpc::Status result;

        // create a connection to the other agent
        std::unique_ptr<remote::ISiteAgent::Stub> stub =
            remote::ISiteAgent::NewStub(GetSiteChannel(secondSite));
        if(stub)
        {
            ClientContext ctx;
            google::protobuf::Empty response;
            LOGTRACE("Calling StartNode on peer " + secondSite);
            // start the next node, passing on the entire path
            result = LogStatus(
                         stub->StartNode(&ctx, *path, &response));
            if(!result.ok())
            {
                LOGERROR("Failed to start the other side: " + secondSite);
            }
        } // if stub
        else
        {
            result = LogStatus(Status(StatusCode::UNAVAILABLE, "Cannot contact next hop"));
        } // else stub

        return result;
    } // StartLeftSide

    grpc::Status SiteAgent::StartSession(std::shared_ptr<grpc::Channel> channel, const remote::SessionDetails& sessionDetails, const std::string& remoteSessionAddress)
    {
        using namespace std;
        using namespace grpc;

        LOGTRACE("Starting session");
        // start exchanging keys
        grpc::ClientContext ctx;
        remote::SessionDetailsTo request;
        google::protobuf::Empty response;
        auto deviceStub = remote::IDevice::NewStub(channel);

        (*request.mutable_details()) = sessionDetails;
        request.set_peeraddress(remoteSessionAddress);
        return deviceStub->RunSession(&ctx, request, &response);

    } // StartRightSide

    grpc::Status SiteAgent::StartNode(grpc::ServerContext*, remote::HopPair& hop, remote::PhysicalPath& myPath)
    {
        LOGTRACE("Looking at hop from " + hop.first().site() + " to " + hop.second().site());

        grpc::Status result;
        bool alreadyConnected = false;

        // if we are the left side of a hop, ie a in [a, b]
        if(AddressIsThisSite(hop.first().site()))
        {
            bool alreadyConnected = false;
            /*lock scope*/
            {
                std::lock_guard<std::mutex> lock(statusCallbackMutex);
                alreadyConnected = otherSites[hop.first().site()].state !=
                                   remote::LinkStatus_State::LinkStatus_State_Inactive;
            }/*lock scope*/

            if(!alreadyConnected)
            {
                // configure the device and set up the controller

                result = PrepHop(hop.first().deviceid(), hop.second().site(), *hop.mutable_params());
                if(result.ok())
                {
                    auto devDetails = FindDeviceDetails(siteDetails, hop.first().deviceid());
                    if(devDetails)
                    {
                        // fill in the missing detgails for the local device
                        hop.mutable_first()->set_deviceaddress(devDetails->controladdress());
                    }
                    else
                    {
                        LOGERROR("Failed to find device connection address");
                    }
                    result = ForwardOnStartNode(&myPath, hop.second().site());
                }

            }
            else
            {
                result = Status(grpc::StatusCode::OK, "Already connected");
                LOGINFO("Already connected to " + hop.first().site());
            }

        } // if left
        else if(AddressIsThisSite(hop.second().site()))
        {
            /*lock scope*/
            {
                std::lock_guard<std::mutex> lock(statusCallbackMutex);
                alreadyConnected = otherSites[hop.second().site()].state !=
                                   remote::LinkStatus_State::LinkStatus_State_Inactive;
            }/*lock scope*/

            if(!alreadyConnected)
            {
                // connect the session controller to the previous hop which should already be setup
                // configure the device and set up the controller
                Status result = PrepHop(hop.second().deviceid(), hop.first().site(), *hop.mutable_params());
                if(result.ok() && devicesInUse[hop.second().deviceid()])
                {
                    // setup the second half of the chain
                    LOGTRACE("Starting session with local device: " + hop.second().deviceaddress() + " and remote device: " +
                             hop.first().deviceaddress());
                    result = StartSession(devicesInUse[hop.second().deviceid()]->channel, hop.params(), hop.first().deviceaddress());
                }
            }
            else
            {
                result = Status(grpc::StatusCode::OK, "Already connected");
                LOGINFO("Already connected to " + hop.second().site());
            }
        } // if right

        // only take down the failed link if its a new link, don't destroy the working link because
        // of a bad request to extend it with a multi-hop link
        if(!alreadyConnected && !result.ok())
        {
            auto localDev = devicesInUse.find(hop.first().deviceid());
            if(localDev != devicesInUse.end())
            {
                // something went wrong, return the device
                localDev->second->Stop();
                devicesInUse.erase(localDev);
            }
        }

        return result;
    }

    grpc::Status SiteAgent::StartNode(grpc::ServerContext* ctx, const remote::PhysicalPath* path, google::protobuf::Empty*)
    {
        using namespace std;
        using namespace grpc;
        {
            std::string pathString;
            for(const auto& element : path->hops())
            {
                pathString += element.first().site() + "<->" + element.second().site() + " | ";
            }
            LOGDEBUG(GetConnectionAddress() + " is starting node with: " + pathString);
        }

        auto pathCopy = *path;

        // default result
        grpc::Status result = Status(StatusCode::NOT_FOUND, "No hops applicable to this site");
        // walk through each hop in the path
        // path example = [ [a, b], [b, c] ]
        for(auto& hopPair : *pathCopy.mutable_hops())
        {
            result = StartNode(ctx, hopPair, pathCopy);
        } // for pairs


        string dest;
        if(result.ok())
        {
            std::vector<std::string> intermediateSites;
            if(AddressIsThisSite(path->hops().begin()->first().site()))
            {
                // the path is forward - we are on the left
                dest = path->hops().rbegin()->second().site();
                // skip the first element, that's our site
                for (int index = 1; index < path->hops().size(); index++)
                {
                    // use the first element to end up with a list of sites in the middle
                    intermediateSites.push_back(path->hops(index).first().site());
                }
            }
            else if(AddressIsThisSite(path->hops().rbegin()->second().site()))
            {
                // the path is backwards - we are on the right
                dest = path->hops().begin()->first().site();
                // skip the first element
                for (int index = path->hops().size() - 1; index > 0; index--)
                {
                    // use the first element to end up with a list of sites in the middle
                    intermediateSites.push_back(path->hops(index).first().site());
                }
            }

            // set the path if we're at ether end
            // the path can be empty
            if(!dest.empty())
            {
                // prepare the keystore
                LOGTRACE("Configuring keystore");
                auto ks = keystoreFactory->GetKeyStore(dest);
                if(ks)
                {
                    ks->SetPath(intermediateSites);
                }
                else
                {
                    LOGERROR("Failed to create keystore");
                    result = Status(StatusCode::INTERNAL, "Failed to create keystore for destination: " + dest);
                }
            }

        } // if result.ok()

        if(result.ok())
        {
            LOGINFO("Node setup complete");
        }
        return result;
    } // StartNode

    grpc::Status SiteAgent::StopNode(const std::string& deviceUri)
    {
        grpc::Status result;

        auto device = devicesInUse.find(deviceUri);
        if(device != devicesInUse.end())
        {
            // stop the process
            device->second->Stop();
            devicesInUse.erase(device);
        }
        else
        {
            result = Status(StatusCode::INVALID_ARGUMENT, "Unknown device");
        }

        return result;
    }

    bool SiteAgent::AddressIsThisSite(const std::string& address)
    {
        bool result = address == GetConnectionAddress();
        if(!result)
        {
            URI myUri(GetConnectionAddress());
            URI addrUri(address);
            if(myUri.GetPort() == addrUri.GetPort())
            {
                result = addrUri.GetHost() == "localhost" || addrUri.GetHost() == "127.0.0.1";
                if(!result)
                {
                    net::IPAddress addrIp;
                    if(net::ResolveAddress(addrUri.GetHost(), addrIp))
                    {
                        const auto hostIps = net::GetHostIPs();
                        result = std::any_of(hostIps.begin(), hostIps.end(), [&addrIp](auto myIp)
                        {
                            return myIp == addrIp;
                        });
                    } // if resolve
                } // if !result
            } // ports match
        } // if !result
        return result;
    } // AddressIsThisSite

    grpc::Status SiteAgent::EndKeyExchange(grpc::ServerContext*,
                                           const remote::PhysicalPath* path, google::protobuf::Empty*)
    {
        using namespace std;
        using namespace grpc;
        grpc::Status result = Status(StatusCode::NOT_FOUND, "No hops applicable to this site");
        {
            std::string pathString;
            for(const auto& element : path->hops())
            {
                pathString += element.first().site() + "<->" + element.second().site() + " | ";
            }
            LOGDEBUG(GetConnectionAddress() + " is stopping node with: " + pathString);
        }

        // stop the session controllers

        bool callNextNode = false;

        // walk through each hop in the path
        // path example = [ [a, b], [b, c] ]
        for(const auto& hopPair : path->hops())
        {


            // if we are the left side of a hop, ie a in [a, b]
            if(AddressIsThisSite(hopPair.first().site()))
            {
                result = StopNode(hopPair.first().deviceid());

                auto otherSide = otherSites.find(hopPair.second().site());
                if(otherSide != otherSites.end())
                {
                    grpc::ClientContext ctx;

                    auto siteStub = remote::ISiteAgent::NewStub(otherSide->second.channel);

                    google::protobuf::Empty response;
                    result =  LogStatus(siteStub->EndKeyExchange(&ctx, *path, &response));

                }
                else
                {
                    LOGWARN("Cant find " + hopPair.second().site() + " to stop it");
                }
            } // if left
            else if(AddressIsThisSite(hopPair.second().site()))
            {
                result = StopNode(hopPair.second().deviceid());
                callNextNode = true;
            } // if right
            else if(callNextNode)
            {
                callNextNode = false;
                auto otherSide = otherSites.find(hopPair.first().site());
                if(otherSide != otherSites.end())
                {
                    grpc::ClientContext ctx;

                    auto siteStub = remote::ISiteAgent::NewStub(otherSide->second.channel);

                    google::protobuf::Empty response;
                    result =  LogStatus(siteStub->EndKeyExchange(&ctx, *path, &response));
                }
                else
                {
                    LOGWARN("Cant find " + hopPair.first().site() + " to stop it");
                }
            }
        } // for pairs

        return result;
    } // EndKeyExchange

    grpc::Status SiteAgent::GetSiteDetails(grpc::ServerContext*, const google::protobuf::Empty*, remote::Site* response)
    {
        *response = siteDetails;
        return grpc::Status();
    } // GetSiteDetails

    std::shared_ptr<grpc::Channel> SiteAgent::GetSiteChannel(const std::string& connectionAddress)
    {
        using namespace grpc;
        std::shared_ptr<grpc::Channel> result = otherSites[connectionAddress].channel;

        if(result == nullptr)
        {
            // unknown address, create a new channel
            result = CreateChannel(connectionAddress, LoadChannelCredentials(myConfig.credentials()));
            otherSites[connectionAddress].channel = result;
        }

        return result;
    }// GetSiteChannel

    grpc::Status SiteAgent::RegisterDevice(grpc::ServerContext*, const remote::ControlDetails* request, google::protobuf::Empty*)
    {
        LOGDEBUG("Device registering: " + request->config().id());
        using namespace std;

        {
            /*lock scope*/
            lock_guard<mutex> lock(siteDetailsMutex);
            (*siteDetails.add_devices()) = *request;

        }/*lock scope*/

        string sideString;
        switch (request->config().side())
        {
        case remote::Side_Type::Side_Type_Alice:
            sideString = "Alice";
            break;
        case remote::Side_Type::Side_Type_Bob:
            sideString = "Bob";
            break;
        default:
            sideString = "Any";
            break;
        }
        const std::vector<std::string> ports(request->config().switchport().begin(), request->config().switchport().end());
        LOGINFO("New " + sideString + " device: " + request->config().id() + " at '" + request->controladdress() + "' on switch '" +
                request->config().switchname() + "' port '" + Join(ports, ",") + "'");

        // register with the internal network manager
        if(internalNetMan)
        {
            LOGINFO("Updating internal network manager");
            LogStatus(internalNetMan->RegisterSite(nullptr, &siteDetails, nullptr));
        }

        // tell everyone we changed the site details
        siteDetailsChanged.notify_all();

        return Status();
    }

    grpc::Status SiteAgent::UnregisterDevice(grpc::ServerContext*, const remote::DeviceID* request, google::protobuf::Empty*)
    {
        LOGDEBUG("Device unregistering: " + request->id());
        using namespace std;
        Status result = Status(StatusCode::NOT_FOUND, "Unknown device " + request->id());

        {
            /*lock scope*/
            lock_guard<mutex> lock(siteDetailsMutex);
            for(int index = 0; index < siteDetails.devices().size(); index++)
            {
                if(siteDetails.devices(index).config().id() == request->id())
                {
                    siteDetails.mutable_devices()->erase(siteDetails.mutable_devices()->begin() + index);
                    result = Status();
                    break; // for
                }
            }
        }/*lock scope*/

        // register with the internal network manager
        if(internalNetMan)
        {
            LOGINFO("Updating internal network manager");
            LogStatus(internalNetMan->RegisterSite(nullptr, &siteDetails, nullptr));
        }

        // tell everyone we changed the site details
        siteDetailsChanged.notify_all();

        return result;
    }

    void SiteAgent::DeviceConnection::Stop()
    {
        keyReaderContext.TryCancel();
        statsContext.TryCancel();

        auto deviceStub = remote::IDevice::NewStub(channel);

        grpc::ClientContext ctx;
        google::protobuf::Empty request;
        google::protobuf::Empty response;
        LogStatus(deviceStub->EndSession(&ctx, request, &response));

        if(readerThread.joinable())
        {
            readerThread.join();
        }
        if(statsThread.joinable())
        {
            statsThread.join();
        }
    }

    void SiteAgent::DeviceConnection::ReadStats(stats::ReportServer* reportServer, std::string siteTo)
    {
        using namespace remote;
        auto statsStub = IReporting::NewStub(channel);
        if(statsStub)
        {
            ReportingFilter filter;
            filter.set_listisexclude(true);
            auto reader = statsStub->GetStatistics(&statsContext, filter);

            SiteAgentReport report;
            // pull stats and pass them on
            while(reader && reader->Read(&report))
            {
                report.mutable_parameters()->insert({"siteTo", siteTo});
                reportServer->StatsReport(report);
            } // while read

            reader->Finish();
        } // if stub
    } // ReadStats

    // ConnectStaticLinks
} // namespace cqp


