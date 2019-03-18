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

#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "QKDInterfaces/Device.pb.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <google/protobuf/util/message_differencer.h>
#include <unordered_map>

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
            myConfig.set_connectionaddress(net::GetHostname() + ":" + std::to_string(myConfig.listenport()));
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
    } // SiteAgent

    SiteAgent::~SiteAgent()
    {
        shutdown = true;
        // trigger the unregistering from the network manager
        siteDetailsChanged.notify_all();

        // disconnect all session controllers
        for(auto& device : devicesInUse)
        {
            device.second->context.TryCancel();
        }
        devicesInUse.clear();

        server->Shutdown();

        if(reportServer)
        {
            if(keystoreFactory)
            {
                keystoreFactory->RemoveReportingCallback(reportServer.get());
            }
        }

        if(netManRegister.joinable())
        {
            netManRegister.join();
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

    void SiteAgent::ProcessKeys(std::shared_ptr<DeviceConnection> connection, std::unique_ptr<PSK> initialKey)
    {
        using namespace std;
        auto deviceStub = remote::IDevice::NewStub(connection->channel);

        if(deviceStub && connection->keySink)
        {
            remote::LocalSettings request;
            remote::RawKeys incommingKeys;
            request.set_initialkey(initialKey->data(), initialKey->size());

            auto reader = deviceStub->WaitForSession(&connection->context, request);

            // TODO this isn't enough to ensure the memory is clean.
            request.clear_initialkey();
            initialKey->clear();
            initialKey.reset();

            while(reader && reader->Read(&incommingKeys))
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
                }
                // send the keys on
                connection->keySink->OnKeyGeneration(move(keys));
            }

            LogStatus(reader->Finish());

        }
        else
        {
            LOGERROR("Faild to WaitForSession, invalid setup");
        }
    }

    grpc::Status SiteAgent::PrepHop(const std::string& deviceId, const std::string& destination, std::string& localSessionAddress, remote::SessionDetails& params)
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
                if(regDevice.id() == deviceId)
                {
                    // store the new device and get the iterator to it for later
                    auto localDev = make_shared<DeviceConnection>();
                    LOGTRACE("Connecting to device control at " + regDevice.controladdress());
                    localDev->channel = grpc::CreateChannel(regDevice.controladdress(), LoadChannelCredentials(myConfig.credentials()));
                    localDev->keySink = keystoreFactory->GetKeyStore(destination);
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

                    localDev->readerThread = thread(&SiteAgent::ProcessKeys, localDev, move(initialPsk));
                    // BANG! need to wait for session address
                    // this is the device we're looking for
                    localSessionAddress = regDevice.sessionaddress();


                    devicesInUse.emplace(deviceId, move(localDev));
                    // The session will be start by the right side as it has all the required details
                    result = Status();
                    break; // for
                }
            }
        }
        else
        {
            LOGTRACE("Hop already active");
        }

        return result;
    } // PrepHop

    grpc::Status SiteAgent::ForwardOnStartNode(const remote::PhysicalPath* path, const std::string& secondSite,
            const std::string& localSessionAddress)
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
            // this allows the next session controller to talk to our session controller
            // TODO: should this be part of the interface?
            ctx.AddMetadata(sessionAddress, localSessionAddress.c_str());
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

    grpc::Status SiteAgent::StartNode(grpc::ServerContext* ctx, remote::HopPair& hop, remote::PhysicalPath& myPath)
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
                std::string localSessionAddress;
                result = PrepHop(hop.first().deviceid(), hop.second().site(), localSessionAddress, *hop.mutable_params());
                if(result.ok())
                {
                    result = ForwardOnStartNode(&myPath, hop.second().site(), localSessionAddress);
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
                std::string remoteSessionAddress;
                // connect the session controller to the previous hop which should already be setup
                // this address currently comes from "AddMetadata" from the client
                // TODO: should this be a real interface?
                auto metaDataIt = ctx->client_metadata().find(sessionAddress);
                if(metaDataIt != ctx->client_metadata().end())
                {
                    std::string localSessionAddress;
                    // BUG: value is not terminated, so using .data() results in extra data at the end.
                    // Copy the string using the length defined in the string
                    remoteSessionAddress.assign(metaDataIt->second.begin(), metaDataIt->second.end());
                    // configure the device and set up the controller
                    Status result = PrepHop(hop.second().deviceid(), hop.first().site(), localSessionAddress, *hop.mutable_params());
                    if(result.ok() && devicesInUse[hop.second().deviceid()])
                    {
                        // setup the second half of the chain
                        LOGTRACE("Starting session to remote session " + remoteSessionAddress);
                        result = StartSession(devicesInUse[hop.second().deviceid()]->channel, hop.params(), remoteSessionAddress);
                    }

                }
                else
                {
                    // assume that the caller is not a site agent and has got the hop direction backwards, i.e:
                    // asking B to setup A->B
                    LOGDEBUG("Backwards hop");
                    result = Status(StatusCode::INVALID_ARGUMENT, "Hop appears to be backwards. Called wrong Site?");
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
                localDev->second->context.TryCancel();
                if(localDev->second->readerThread.joinable())
                {
                    localDev->second->readerThread.join();
                }
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
            device->second->context.TryCancel();
            if(device->second->readerThread.joinable())
            {
                device->second->readerThread.join();
            }
            devicesInUse.erase(device);
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

        // walk through each hop in the path
        // path example = [ [a, b], [b, c] ]
        for(const auto& hopPair : path->hops())
        {
            // if we are the left side of a hop, ie a in [a, b]
            if(AddressIsThisSite(hopPair.first().site()))
            {
                result = StopNode(hopPair.first().deviceid());

            } // if left
            else if(AddressIsThisSite(hopPair.second().site()))
            {
                result = StopNode(hopPair.second().deviceid());
            } // if right
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

    grpc::Status SiteAgent::RegisterDevice(grpc::ServerContext*, const remote::DeviceConfig* request, google::protobuf::Empty*)
    {
        using namespace std;

        {
            /*lock scope*/
            lock_guard<mutex> lock(siteDetailsMutex);
            (*siteDetails.add_devices()) = *request;

        }/*lock scope*/

        // TODO: fix static hops

        // tell everyone we changed the site details
        siteDetailsChanged.notify_all();

        return Status();
    }

    grpc::Status SiteAgent::UnregisterDevice(grpc::ServerContext*, const remote::DeviceID* request, google::protobuf::Empty*)
    {
        using namespace std;
        Status result = Status(StatusCode::NOT_FOUND, "Unknown device " + request->id());

        {
            /*lock scope*/
            lock_guard<mutex> lock(siteDetailsMutex);
            for(int index = 0; index < siteDetails.devices().size(); index++)
            {
                if(siteDetails.devices(index).id() == request->id())
                {
                    siteDetails.mutable_devices()->erase(siteDetails.mutable_devices()->begin() + index);
                    result = Status();
                    break; // for
                }
            }
        }/*lock scope*/
        // tell everyone we changed the site details
        siteDetailsChanged.notify_all();

        return result;
    }

    void SiteAgent::ConnectStaticLinks()
    {
        using namespace std;

        for(const auto& staticHop : myConfig.statichops())
        {
            LOGINFO("Connecting to static peer " + staticHop.hops().rbegin()->second().site());
            grpc::ServerContext ctx;
            google::protobuf::Empty response;

            LogStatus(StartNode(&ctx, &staticHop, &response));
        }
    } // ConnectStaticLinks
} // namespace cqp


