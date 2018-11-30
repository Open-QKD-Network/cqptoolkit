/*!
* @file
* @brief TunnelManager
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/3/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Controller.h"
#include "Drivers/IDQSequenceLauncher.h"
#include "Drivers/Clavis.h"
#include "CQPToolkit/Net/Interface.h"
#include "CQPToolkit/Util/Util.h"
#include "CQPAlgorithms/Datatypes/UUID.h"
#include "CQPToolkit/Util/URI.h"
#include "CQPToolkit/Tunnels/TunnelBuilder.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include "QKDInterfaces/IKeyFactory.grpc.pb.h"
#include "CQPToolkit/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
using cqp::remote::tunnels::ControllerDetails;
using grpc::Status;
using grpc::StatusCode;
using google::protobuf::Empty;

namespace cqp
{
    namespace tunnels
    {

        Controller::Controller(const remote::tunnels::ControllerDetails& initialSettings)
            : settings(initialSettings)
        {
            using remote::tunnels::ControllerDetails;
            clientCreds = LoadChannelCredentials(settings.credentials());
            serverCreds = LoadServerCredentials(settings.credentials());

            if(settings.localKeyFactory_case() == ControllerDetails::LocalKeyFactoryCase::kLocalKeyFactoryUri)
            {
                keyStoreFactoryUri = settings.localkeyfactoryuri();
                keyFactoryChannel = grpc::CreateChannel(keyStoreFactoryUri, clientCreds);
            }

            LOGINFO("Tunnelling controller started with ID: " + settings.id());
        }

        Status Controller::GetSupportedSchemes(grpc::ServerContext*, const google::protobuf::Empty*, remote::tunnels::EncryptionSchemes* response)
        {
            response->add_modes(Modes::None);
            response->add_modes(Modes::GCM);

            response->add_submodes(SubModes::None);
            response->add_submodes(SubModes::Tables64K);
            response->add_submodes(SubModes::Tables2K);

            response->add_blockcyphers(BlockCiphers::None);
            response->add_blockcyphers(BlockCiphers::AES);

            response->add_numbergenerators(RandomNumberGenerators::Any);
            response->add_numbergenerators(RandomNumberGenerators::RDRAND);
            response->add_numbergenerators(RandomNumberGenerators::OSX917);
            response->add_numbergenerators(RandomNumberGenerators::SWRNG);

            response->keysizes(KeySizes::Key256);
            response->keysizes(KeySizes::Key128);
            return Status();
        }

        void Controller::GetControllerSettings(ControllerDetails& response)
        {
            if(!UUID::IsValid(settings.id()))
            {
                // create a new id
                settings.set_id(UUID());
            }
            response = settings;
        }

        Status Controller::GetControllerSettings(grpc::ServerContext*, const Empty*, ControllerDetails* response)
        {
            GetControllerSettings(*response);
            return Status();
        }

        grpc::Status Controller::ModifyTunnel(grpc::ServerContext*, const remote::tunnels::Tunnel* request, Empty*)
        {
            std::lock_guard<std::mutex> lock(settingsMutex);
            (*settings.mutable_tunnels())[request->name()] = *request;
            return Status();
        }

        grpc::Status Controller::DeleteTunnel(grpc::ServerContext*, const google::protobuf::StringValue* request, Empty*)
        {
            Status result = Status(StatusCode::INVALID_ARGUMENT, "Tunnel name not found");
            std::lock_guard<std::mutex> lock(settingsMutex);
            auto it = settings.mutable_tunnels()->find(request->value());
            if(it != settings.mutable_tunnels()->end())
            {
                settings.mutable_tunnels()->erase(it);
                result = Status();
                LOGINFO("Tunnel deleted");
            }
            return result;
        }

        std::shared_ptr<grpc::Channel> Controller::FindController(const remote::tunnels::Tunnel& tun)
        {
            using remote::tunnels::Tunnel;
            std::shared_ptr<grpc::Channel> result;
            switch (tun.remoteController_case())
            {
            case Tunnel::RemoteControllerCase::kRemoteControllerUri:
            {
                auto otherControllerIt = endpointsByName.find(tun.remotecontrolleruri());
                if(otherControllerIt != endpointsByName.end())
                {
                    result = otherControllerIt->second;
                }
                else if(!tun.remotecontrolleruri().empty())
                {
                    result = grpc::CreateChannel(tun.remotecontrolleruri(), clientCreds);
                    endpointsByName[tun.remotecontrolleruri()] = result;
                }
            }
            break;
            case Tunnel::RemoteControllerCase::kRemoteControllerUuid:
            {
                auto otherControllerIt = endpointsByID.find(tun.remotecontrolleruuid());
                if(otherControllerIt != endpointsByID.end())
                {
                    result = otherControllerIt->second;
                }
            }
            break;

            case Tunnel::RemoteControllerCase::REMOTECONTROLLER_NOT_SET:
                break;
            }

            return result;
        }

        Status Controller::StartTunnel(grpc::ServerContext*, const google::protobuf::StringValue* request, Empty*)
        {
            remote::tunnels::CompleteTunnelRequest tunSettings;
            Status result;
            LOGDEBUG("Waiting for keystore");

            if(!WaitForKeyStore())
            {
                result = Status(StatusCode::UNAVAILABLE, "Local keystore not available");
            }

            if(result.ok())
            {
                LOGDEBUG("Keystore ready");

                std::lock_guard<std::mutex> lock(settingsMutex);
                auto settingsIt = settings.tunnels().find(request->value());
                if(settingsIt != settings.tunnels().end())
                {
                    (*tunSettings.mutable_tunnel()) = settingsIt->second;
                    tunSettings.set_startkeystore(keyStoreFactoryUri);
                    result = Status();
                }
                else
                {
                    result = Status(StatusCode::INVALID_ARGUMENT, "No settings found for tunnel " + request->value());
                }
            }

            if(result.ok())
            {
                auto builderIt = tunnelBuilders.find(request->value());
                if(builderIt == tunnelBuilders.end())
                {
                    int tunnelListenPort = static_cast<int>(tunSettings.tunnel().startnode().localchannelport());
                    auto newBuilder = new TunnelBuilder(tunSettings.tunnel().encryptionmethod(), tunnelListenPort,
                                                        LoadServerCredentials(settings.credentials()),
                                                        LoadChannelCredentials(settings.credentials()));
                    tunnelBuilders[request->value()].reset(newBuilder);
                    // tell the remote node how to connect to this node
                    tunSettings.mutable_tunnel()->mutable_endnode()->set_channeluri(net::GetHostname() + ":" + std::to_string(tunnelListenPort));
                    // try and find the controller by ID first
                    std::shared_ptr<grpc::Channel> otherController = FindController(tunSettings.tunnel());

                    while(!otherController)
                    {
                        LOGINFO("Waiting for controller...");
                        std::unique_lock<std::mutex> lock(controllerDetectedMutex);
                        controllerDetectedCv.wait(lock, [&]()
                        {
                            otherController = FindController(tunSettings.tunnel());
                            return otherController != nullptr;
                        });
                    }

                    if(otherController)
                    {
                        LOGDEBUG("Found controller");
                        auto peer = remote::tunnels::ITunnelServer::NewStub(otherController);
                        grpc::ClientContext ctx;
                        remote::tunnels::CompleteTunnelResponse response;
                        LOGTRACE("Calling CompleteTunnel on peer");
                        result = LogStatus(peer->CompleteTunnel(&ctx, tunSettings, &response));
                        // get the connection address from the other end
                        tunSettings.mutable_tunnel()->mutable_startnode()->set_channeluri(response.encryptedconnectionuri());

                        if(result.ok())
                        {
                            LOGDEBUG("Configuring endpoint");
                            // start this side
                            result = newBuilder->ConfigureEndpoint(tunSettings.tunnel().startnode(), keyFactoryChannel,
                                                                   response.keystoreaddress(),
                                                                   tunSettings.tunnel().keylifespan());
                        }
                    }
                    else
                    {
                        result = Status(StatusCode::NOT_FOUND, "Cannot find controller: " + tunSettings.tunnel().remotecontrolleruri() + " with id: " + tunSettings.tunnel().remotecontrolleruuid());
                    }
                }
                else
                {
                    result = Status(StatusCode::ALREADY_EXISTS, "Tunnel already started");
                }
            }

            return result;
        } // StartTunnel

        Status Controller::StopTunnel(grpc::ServerContext*, const google::protobuf::StringValue* request, Empty*)
        {
            Status result(StatusCode::NOT_FOUND, "Unknown tunnel");
            auto builderIt  = tunnelBuilders.find(request->value());
            if(builderIt != tunnelBuilders.end())
            {
                builderIt->second->Shutdown();
                tunnelBuilders.erase(builderIt);
                result = Status();
                LOGINFO("Tunnel stopped");
            }
            return result;
        } // StopTunnel

        Status Controller::CompleteTunnel(grpc::ServerContext*, const remote::tunnels::CompleteTunnelRequest* request, remote::tunnels::CompleteTunnelResponse* response)
        {
            LOGTRACE("Called");
            Status result;

            LOGDEBUG("Waiting for keystore...");
            if(WaitForKeyStore())
            {
                LOGDEBUG("Keystore ready");
                auto builderIt = tunnelBuilders.find(request->tunnel().name());
                if(builderIt == tunnelBuilders.end())
                {
                    int tunnelListenPort = static_cast<int>(request->tunnel().endnode().localchannelport());
                    LOGTRACE("Creating tunnel builder");
                    auto newBuilder = new TunnelBuilder(request->tunnel().encryptionmethod(), tunnelListenPort,
                                                        LoadServerCredentials(settings.credentials()),
                                                        LoadChannelCredentials(settings.credentials()));
                    // tell the remote node how to connect to this node
                    response->set_encryptedconnectionuri(net::GetHostname() + ":" + std::to_string(tunnelListenPort));

                    // start this side
                    LOGTRACE("Configuring endpoint");
                    result = newBuilder->ConfigureEndpoint(request->tunnel().endnode(), keyFactoryChannel,
                                                           request->startkeystore(),
                                                           request->tunnel().keylifespan());

                    /*lock scope*/
                    {
                        std::lock_guard<std::mutex> lock(settingsMutex);
                        response->set_keystoreaddress(keyStoreFactoryUri);
                    }/*lock scope*/

                    if(result.ok())
                    {
                        LOGINFO("Tunnel setup complete");
                    }
                }
                else
                {
                    result = Status(StatusCode::ALREADY_EXISTS, "Tunnel already started");
                }
            }
            else
            {
                result = Status(StatusCode::UNAVAILABLE, "Local keystore not available");
            }

            return result;
        } // CompleteTunnel


        Controller::~Controller()
        {
            StopAllTunnels();
        } // ~Controller

        void Controller::StartAllTunnels()
        {
            for(const auto& tun : settings.tunnels())
            {
                grpc::ServerContext ctx;
                google::protobuf::StringValue request;
                request.set_value(tun.first);
                Empty response;
                LogStatus(StartTunnel(&ctx, &request, &response));
            }
        } // StartAllTunnels

        void Controller::StopAllTunnels()
        {
            tunnelBuilders.clear();
        } // StopAllTunnels

        void Controller::OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts&)
        {
            bool endpointsChanged = false;
            for(const auto& service : newServices)
            {
                if(service.second.interfaces.contains(remote::tunnels::ITunnelServer::service_full_name()))
                {
                    std::string serviceUri = service.second.host + ":" + std::to_string(service.second.port);
                    auto otherControllerIt = endpointsByName.find(serviceUri);
                    if(otherControllerIt == endpointsByName.end())
                    {
                        endpointsByName[serviceUri] = grpc::CreateChannel(serviceUri, clientCreds);
                        endpointsChanged = true;
                    }

                    otherControllerIt = endpointsByID.find(service.second.id);
                    if(otherControllerIt == endpointsByID.end() && !service.second.id.empty())
                    {
                        endpointsByID[service.second.id] = endpointsByName[serviceUri];
                        endpointsChanged = true;
                    }
                }

                using remote::tunnels::ControllerDetails;

                if(!keyFactoryChannel && // don't have the key factory yet
                        service.second.interfaces.contains(remote::IKeyFactory::service_full_name()) && // this is a key factory
                        settings.localKeyFactory_case() == ControllerDetails::LocalKeyFactoryCase::kLocalKeyFactoryUuid && // we have the id for the key factory
                        service.second.id == settings.localkeyfactoryuuid()) // this is the key factory we're looking for
                {
                    keyStoreFactoryUri = service.second.host + ":" + std::to_string(service.second.port);
                    keyFactoryChannel = grpc::CreateChannel(keyStoreFactoryUri, clientCreds);
                    endpointsChanged = true;
                }
            }

            if(endpointsChanged)
            {
                controllerDetectedCv.notify_all();
            }
        }

        bool Controller::WaitForKeyStore(std::chrono::milliseconds timeout)
        {
            using std::chrono::milliseconds;
            bool result = keyFactoryChannel != nullptr;

            if(!keyFactoryChannel)
            {
                LOGINFO("Waiting for Keystore factory...");
                std::unique_lock<std::mutex> lock(controllerDetectedMutex);
                if(timeout > milliseconds(0))
                {
                    result = controllerDetectedCv.wait_for(lock, timeout, [&]()
                    {
                        return keyFactoryChannel != nullptr;
                    });
                    if(result)
                    {
                        LOGINFO("Keystore found.");
                    }
                }
                else
                {
                    controllerDetectedCv.wait(lock, [&]()
                    {
                        return keyFactoryChannel != nullptr;
                    });
                    LOGINFO("Keystore found.");
                    result = true;
                }
            }

            return result;
        }
    } // namespace tunnels
} // namespace cqp
