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
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "CQPToolkit/QKDDevices/DeviceFactory.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "KeyManagement/Net/ServiceDiscovery.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "Algorithms/Statistics/StatisticsLogger.h"
#include "Algorithms/Net/DNS.h"
#include "KeyManagement/KeyStores/HSMStore.h"
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include "CQPToolkit/Auth/AuthUtil.h"
#include <thread>
#include "KeyManagement/KeyStores/BackingStoreFactory.h"

#if defined(SQLITE3_FOUND)
    #include "KeyManagement/KeyStores/FileStore.h"
#endif
namespace cqp
{

    using grpc::Status;
    using grpc::StatusCode;

    void SiteAgent::RegisterWithNetMan()
    {
        if(server && netMan)
        {
            grpc::Status registerResult;
            do
            {
                google::protobuf::Empty response;
                grpc::ClientContext ctx;
                registerResult = LogStatus(
                                     netMan->RegisterSite(&ctx, siteDetails, &response), "Failed to register site with Network Manager");
            }
            while(!registerResult.ok());
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

        if(!config.netmanuri().empty())
        {
            LOGINFO("Connecting to Network Manager: " + config.netmanuri());
            netmanChannel = grpc::CreateChannel(config.netmanuri(), LoadChannelCredentials(config.credentials()));
            netMan = remote::INetworkManager::NewStub(netmanChannel);
        }
        deviceFactory.reset(new DeviceFactory(LoadChannelCredentials(config.credentials())));

        keystoreFactory.reset(new keygen::KeyStoreFactory(LoadChannelCredentials(config.credentials()),
                                                          keygen::BackingStoreFactory::CreateBackingStore(myConfig.backingstoreurl())));
        reportServer.reset(new stats::ReportServer());

        // attach the reporting to the device factory so it link them when creating devices
        deviceFactory->AddReportingCallback(reportServer.get());
        keystoreFactory->AddReportingCallback(reportServer.get());
        // for debug
        //deviceFactory->AddReportingCallback(statsLogger.get());

        // build devices
        for(const auto& devaddr : myConfig.deviceurls())
        {
            LOGTRACE("Configuring device " + devaddr);
            auto dev = deviceFactory->CreateDevice(devaddr);
            if(dev == nullptr)
            {
                LOGERROR("Invalid device: " + devaddr);
            }
            else
            {
                // Get the address from the device so that it can fill in any missing details
                // which might be needed later
                auto details = siteDetails.mutable_devices()->Add();
                *details = dev->GetDeviceDetails();
            }
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
            builder.AddListeningPort("0.0.0.0:" + std::to_string(myConfig.listenport()),
                                     LoadServerCredentials(config.credentials()), &listenPort);
        }
        else
        {
            builder.AddListeningPort(myConfig.bindaddress() + ":" + std::to_string(myConfig.listenport()),
                                     LoadServerCredentials(config.credentials()), &listenPort);
        }


        // Register services
        builder.RegisterService(static_cast<remote::ISiteAgent::Service*>(this));
        builder.RegisterService(static_cast<remote::ISiteDetails::Service*>(this));
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

        (*siteDetails.mutable_url()) = myConfig.connectionaddress();
        // tell the key store factory our address now we have it
        keystoreFactory->SetSiteAddress(GetConnectionAddress());

    } // SiteAgent

    SiteAgent::~SiteAgent()
    {
        if(netMan)
        {
            // un-register from the network manager
            google::protobuf::Empty response;
            remote::SiteAddress siteAddress;
            (*siteAddress.mutable_url()) = GetConnectionAddress();
            grpc::ClientContext ctx;
            grpc::Status result = LogStatus(
                                      netMan->UnregisterSite(&ctx, siteAddress, &response));
        }

        // disconnect all session controllers
        for(const auto& dev : devicesInUse)
        {
            auto controller = dev.second->GetSessionController();
            if(controller)
            {
                auto pub = dev.second->GetKeyPublisher();
                if(pub)
                {
                    pub->Detatch();
                }
                controller->EndSession();
            }
            deviceFactory->ReturnDevice(dev.second);
        }
        devicesInUse.clear();

        server->Shutdown();

        if(reportServer)
        {
            if(deviceFactory)
            {
                deviceFactory->RemoveReportingCallback(reportServer.get());
            }
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

    grpc::Status SiteAgent::PrepHop(const std::string& deviceId, const std::string& destination, ISessionController*& controller)
    {
        using namespace std;
        using namespace grpc;
        Status result;
        LOGTRACE("");

        // get the device we've been told to use
        std::shared_ptr<IQKDDevice> localDev = deviceFactory->UseDeviceById(deviceId);
        if(localDev)
        {
            controller = localDev->GetSessionController();

            if(controller)
            {
                LOGTRACE("Starting controller");
                // start the controller so that it can receive connections from the other controller
                result = LogStatus(
                             controller->StartServer());
                if(result.ok())
                {
                    LOGTRACE("Attaching keystore");
                    // attach the device key publisher to the key store for this hop
                    shared_ptr<keygen::KeyStore> ks = keystoreFactory->GetKeyStore(destination);
                    IKeyPublisher* pub = localDev->GetKeyPublisher();
                    if(pub)
                    {
                        LOGTRACE("Adding keystore");
                        // from know on, any key which is produced by this device will
                        // be sent to this key store.
                        pub->Attach(ks.get());
                    }
                    else
                    {
                        result = LogStatus(Status(StatusCode::INTERNAL, "Invalid key publisher"));
                    }
                } // if StartServer == ok
            } // if controller
            else
            {
                result = LogStatus(Status(StatusCode::INTERNAL, "Invalid session controller"));
            } // else controller

            if(result.ok())
            {
                // mark the device as in use
                devicesInUse[deviceId] = localDev;
            }
            else
            {
                // something went wrong, return the device
                deviceFactory->ReturnDevice(localDev);
            }
        } // if localDev
        else
        {
            auto devIt = devicesInUse.find(deviceId);
            if(devIt != devicesInUse.end())
            {
                // this is not an error, we've just already started this connection
                controller = devIt->second->GetSessionController();
            } // if(devIt
            else
            {
                result = LogStatus(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                                "Unknown device: " + deviceId));
                LOGERROR("Failed to find local device " + deviceId);
            } // else devIt
        }  // else localDev

        if(result.ok())
        {
            SendStatusUpdate(destination, remote::LinkStatus_State::LinkStatus_State_Connecting);
        }
        return result;
    } // PrepHop

    grpc::Status SiteAgent::StartLeftSide(const remote::PhysicalPath* path, const remote::HopPair& hopPair)
    {
        using namespace std;
        using namespace grpc;
        LOGTRACE("");
        bool alreadyConnected = false;
        Status result;
        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(statusCallbackMutex);
            alreadyConnected = otherSites[hopPair.second().site()].state !=
                               remote::LinkStatus_State::LinkStatus_State_Inactive;
        }/*lock scope*/

        ISessionController* controller = nullptr;

        const string& localDeviceId = hopPair.first().deviceid();

        if(alreadyConnected)
        {
            result = Status(grpc::StatusCode::OK, "Already connected");
            LOGINFO("Already connected to " + hopPair.second().site());
            auto device = devicesInUse.find(localDeviceId);
            // get the existing controller so we can find it's connection address
            if(device != devicesInUse.end())
            {
                controller = device->second->GetSessionController();
            }
        } else
        {
            // configure the device and set up the controller
            result = PrepHop(localDeviceId, hopPair.second().site(), controller);
        }

        if(result.ok() && controller)
        {
            // create a connection to the other agent
            std::unique_ptr<remote::ISiteAgent::Stub> stub =
                remote::ISiteAgent::NewStub(GetSiteChannel(hopPair.second().site()));
            if(stub)
            {
                ClientContext ctx;
                google::protobuf::Empty response;
                // this allows the next session controller to talk to our session controller
                // TODO: should this be part of the interface?
                ctx.AddMetadata(sessionAddress, controller->GetConnectionAddress());
                LOGTRACE("Calling StartNode on peer");
                // start the next node, passing on the entire path
                result = LogStatus(
                             stub->StartNode(&ctx, *path, &response));
                if(!result.ok())
                {
                    LOGERROR("Failed to start the other side: " + hopPair.second().site());
                }
            } // if stub
            else
            {
                result = LogStatus(Status(StatusCode::UNAVAILABLE, "Cannot contact next hop"));
            } // else stub
        } else {
            LOGERROR("Failed to prepare device and/or controller");
            result = LogStatus(Status(StatusCode::INTERNAL, "Failed to prepare device and/or controller"));
        } // if result.ok() && controller

        // only take down the failed link if its a new link, don't destroy the working link because
        // of a bad request to extend it with a multi-hop link
        if(!alreadyConnected && !result.ok())
        {
            auto localDev = devicesInUse.find(hopPair.first().deviceid());
            if(localDev != devicesInUse.end())
            {
                // something went wrong, return the device
                deviceFactory->ReturnDevice(localDev->second);
                devicesInUse.erase(localDev);
            }

            auto kp = localDev->second->GetKeyPublisher();
            if(kp)
            {
                // disconnect the key store from the publisher
                kp->Detatch();
            }
        }
        return result;
    } // StartLeftSide

    grpc::Status SiteAgent::StartRightSide(grpc::ServerContext*, const remote::HopPair& hopPair, const std::string& remoteSessionAddress)
    {
        using namespace std;
        using namespace grpc;
        LOGTRACE("");
        bool alreadyConnected = false;
        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(statusCallbackMutex);
            alreadyConnected = otherSites[hopPair.second().site()].state !=
                               remote::LinkStatus_State::LinkStatus_State_Inactive;
        }/*lock scope*/

        Status result;

        if(alreadyConnected)
        {
            result = Status(grpc::StatusCode::OK, "Already connected");
            LOGINFO("Already connected to " + hopPair.second().site());
        } else
        {
            ISessionController* controller = nullptr;
            // configure the device and set up the controller
            Status result = PrepHop(hopPair.second().deviceid(), hopPair.first().site(), controller);

            otherSites[hopPair.first().site()].channel = nullptr;

            if(result.ok() && controller)
            {
                LOGTRACE("Start controller and connecting to peer");
                // Connect the controller to the left side controller
                result = LogStatus(
                             controller->StartServerAndConnect(remoteSessionAddress));
                if(result.ok())
                {
                    LOGTRACE("Starting session");
                    // start exchanging keys
                    result = controller->StartSession(hopPair.params());
                }
            }

            if(!result.ok())
            {
                auto localDev = devicesInUse.find(hopPair.second().deviceid());
                if(localDev != devicesInUse.end())
                {
                    // something went wrong, return the device
                    deviceFactory->ReturnDevice(localDev->second);
                    devicesInUse.erase(localDev);
                }
                auto kp = localDev->second->GetKeyPublisher();
                if(kp)
                {
                    // disconnect the key store from the publisher
                    kp->Detatch();
                }
            }
        }

        return result;
    } // StartRightSide

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

        // default result
        grpc::Status result = Status(StatusCode::NOT_FOUND, "No hops applicable to this site");
        // walk through each hop in the path
        // path example = [ [a, b], [b, c] ]
        for(const auto& hopPair : path->hops())
        {

            // if we are the left side of a hop, ie a in [a, b]
            if(AddressIsThisSite(hopPair.first().site()))
            {
                result = StartLeftSide(path, hopPair);

            } // if left
            else if(AddressIsThisSite(hopPair.second().site()))
            {

                std::string remoteSessionAddress;
                // connect the session controller to the previous hop which should already be setup
                // this address currently comes from "AddMetadata" from the client
                // TODO: should this be a real interface?
                auto metaDataIt = ctx->client_metadata().find(sessionAddress);
                if(metaDataIt != ctx->client_metadata().end())
                {
                    // BUG: value is not terminated, so using .data() results in extra data at the end.
                    // Copy the string using the length defined in the string
                    remoteSessionAddress.assign(metaDataIt->second.begin(), metaDataIt->second.end());
                    // setup the second half of the chain
                    result = StartRightSide(ctx, hopPair, remoteSessionAddress);
                }
                else
                {
                    // assume that the caller is not a site agent and has got the hop direction backwards, i.e:
                    // asking B to setup A->B
                    LOGDEBUG("Reversing backwards hop");
                    remote::HopPair reversedHop;
                    *reversedHop.mutable_first() = hopPair.second();
                    *reversedHop.mutable_second() = hopPair.first();
                    // carry on with the hops reversed
                    result = StartLeftSide(path, reversedHop);
                }
            } // if right
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
            SendStatusUpdate(dest, remote::LinkStatus_State::LinkStatus_State_ConnectionEstablished);
            LOGINFO("Node setup complete");
        }
        else
        {
            SendStatusUpdate(dest, remote::LinkStatus_State::LinkStatus_State_Inactive, result);
        }
        return result;
    } // StartNode

    grpc::Status SiteAgent::StopNode(const std::string& deviceUri)
    {
        grpc::Status result;

        auto device = devicesInUse.find(deviceUri);
        if(device != devicesInUse.end())
        {
            auto controller = device->second->GetSessionController();
            if(controller)
            {
                auto pub = device->second->GetKeyPublisher();
                if(pub)
                {
                    // disconnect the device from the keystore
                    pub->Detatch();
                }
                controller->EndSession();
            }

            deviceFactory->ReturnDevice(device->second);
            devicesInUse.erase(device);
        }

        return result;
    }

    void SiteAgent::SendStatusUpdate(const std::string& destination, remote::LinkStatus_State state, grpc::Status status)
    {
        remote::LinkStatus statusUpdate;
        statusUpdate.set_siteto(destination);
        statusUpdate.set_state(state);
        statusUpdate.set_errorcode(static_cast<uint64_t>(status.error_code()));

        std::lock_guard<std::mutex> lock(statusCallbackMutex);
        otherSites[destination].state = state;
        for(const auto& cb : statusCallbacks)
        {
            cb.second(statusUpdate);
        }
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
                        result = std::any_of(hostIps.begin(), hostIps.end(), [&addrIp](auto myIp) {
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
                SendStatusUpdate(hopPair.first().site(), remote::LinkStatus_State::LinkStatus_State_Inactive);

            } // if left
            else if(AddressIsThisSite(hopPair.second().site()))
            {
                result = StopNode(hopPair.second().deviceid());
                SendStatusUpdate(hopPair.second().site(), remote::LinkStatus_State::LinkStatus_State_Inactive);
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

    grpc::Status SiteAgent::GetLinkStatus(grpc::ServerContext* ctx, const google::protobuf::Empty*, ::grpc::ServerWriter<cqp::remote::LinkStatus>* writer)
    {
        using namespace cqp::remote;
        grpc::Status result;
        bool keepGoing = true;
        uint64_t myFuncIndex = 0;


        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(statusCallbackMutex);
            myFuncIndex = statusCounter++;
            statusCallbacks[myFuncIndex] = [&](const cqp::remote::LinkStatus& newStatus)
            {
                try
                {
                    keepGoing = writer->Write(newStatus);
                }
                catch (const std::exception& e)
                {
                    keepGoing = false;
                    LOGERROR(e.what());
                }
            };

            // send current state
            for(const auto& site : otherSites)
            {
                cqp::remote::LinkStatus status;
                status.set_siteto(site.first);
                status.set_state(site.second.state);
                writer->Write(status);
            } // for pairs

        }/*lock scope*/

        while(!ctx->IsCancelled() && keepGoing)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        };

        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(statusCallbackMutex);
            statusCallbacks.erase(myFuncIndex);
        }/*lock scope*/

        return result;
    } // GetLinkStatus

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


