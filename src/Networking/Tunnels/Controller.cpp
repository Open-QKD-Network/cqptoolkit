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
#include "Algorithms/Net/Devices/Interface.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/UUID.h"
#include "Algorithms/Datatypes/URI.h"
#include "Networking/Tunnels/TunnelBuilder.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include "QKDInterfaces/IKeyFactory.grpc.pb.h"
#include "Algorithms/Net/DNS.h"
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
                LOGDEBUG("using keystore: " + settings.localkeyfactoryuri());
                keyStoreFactoryUri = settings.localkeyfactoryuri();
                keyFactoryChannel = grpc::CreateChannel(keyStoreFactoryUri, clientCreds);
            }

            if(settings.id().empty())
            {
                settings.set_id(UUID());
            }
            LOGINFO("Tunnelling controller started with ID: " + settings.id());
            LOGDEBUG("I have " + std::to_string(settings.tunnels().size()) + " tunnels defined:");
            for(const auto &tun: settings.tunnels())
            {
                LOGDEBUG(tun.first + ": " + tun.second.name());
            }
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

        void Controller::ModifyTunnel(const remote::tunnels::Tunnel& tunnel)
        {
            std::lock_guard<std::mutex> lock(settingsMutex);
            (*settings.mutable_tunnels())[tunnel.name()] = tunnel;

        }

        Status Controller::GetControllerSettings(grpc::ServerContext*, const Empty*, ControllerDetails* response)
        {
            GetControllerSettings(*response);
            return Status();
        }

        grpc::Status Controller::ModifyTunnel(grpc::ServerContext*, const remote::tunnels::Tunnel* request, Empty*)
        {
            ModifyTunnel(*request);
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
                    LOGDEBUG("Connecting to " + tun.remotecontrolleruri());
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

        Status Controller::StartTunnel(const std::string &name)
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
                auto settingsIt = settings.tunnels().find(name);
                if(settingsIt != settings.tunnels().end())
                {
                    (*tunSettings.mutable_tunnel()) = settingsIt->second;
                    tunSettings.set_startkeystore(keyStoreFactoryUri);
                    result = Status();
                }
                else
                {
                    result = Status(StatusCode::INVALID_ARGUMENT, "No settings found for tunnel " + name);
                }
            }

            if(result.ok())
            {
                auto builderIt = tunnelBuilders.find(name);
                if(builderIt == tunnelBuilders.end())
                {
                    if(tunSettings.tunnel().encryptionmethod().mode().empty())
                    {
                        LOGDEBUG("Defaulting encryption mode to " + tunnels::Modes::GCM);
                        tunSettings.mutable_tunnel()->mutable_encryptionmethod()->set_mode(tunnels::Modes::GCM);
                    }

                    if(tunSettings.tunnel().encryptionmethod().mode() == tunnels::Modes::GCM &&
                            tunSettings.tunnel().encryptionmethod().submode().empty())
                    {
                        LOGDEBUG("Defaulting sub mode to " + tunnels::SubModes::Tables2K);
                        tunSettings.mutable_tunnel()->mutable_encryptionmethod()->set_submode(tunnels::SubModes::Tables2K);
                    }

                    if(tunSettings.tunnel().encryptionmethod().blockcypher().empty())
                    {
                        LOGDEBUG("Defaulting block cypher to " + tunnels::BlockCiphers::AES);
                        tunSettings.mutable_tunnel()->mutable_encryptionmethod()->set_blockcypher(tunnels::BlockCiphers::AES);
                    }

                    if(tunSettings.tunnel().encryptionmethod().keysizebytes() <= 0)
                    {
                        tunSettings.mutable_tunnel()->mutable_encryptionmethod()->set_keysizebytes(32);
                    }

                    auto newBuilder = std::make_shared<TunnelBuilder>(tunSettings.tunnel().encryptionmethod(),
                                      LoadChannelCredentials(settings.credentials()));
                    tunnelBuilders.emplace(name, newBuilder);

                    // try and find the controller by ID first
                    std::shared_ptr<grpc::Channel> otherController = FindController(tunSettings.tunnel());

                    while(!otherController)
                    {
                        switch(tunSettings.tunnel().remoteController_case())
                        {
                        case remote::tunnels::Tunnel::RemoteControllerCase::kRemoteControllerUri:
                            LOGINFO("Waiting for controller for " + tunSettings.tunnel().name() + " at " +
                                    tunSettings.tunnel().remotecontrolleruri() + "...");
                            break;

                        case remote::tunnels::Tunnel::RemoteControllerCase::kRemoteControllerUuid:
                            LOGINFO("Waiting for controller for " + tunSettings.tunnel().name() + " at " +
                                    tunSettings.tunnel().remotecontrolleruuid() + "...");
                            break;
                        case remote::tunnels::Tunnel::RemoteControllerCase::REMOTECONTROLLER_NOT_SET:
                            LOGERROR("unknown remote address");
                            break;
                        }

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
                        tunSettings.mutable_tunnel()->set_remoteencryptedlistenaddress(response.encryptedconnectionuri());

                        if(result.ok())
                        {
                            LOGDEBUG("Configuring endpoint");
                            // start this side
                            result = newBuilder->ConfigureEndpoint(tunSettings.tunnel().startnode(), keyFactoryChannel,
                                                                   response.keystoreaddress(),
                                                                   tunSettings.tunnel().keylifespan());
                            if(result.ok())
                            {
                                newBuilder->StartTransfer(tunSettings.tunnel().remoteencryptedlistenaddress());
                            }
                            else
                            {
                                LOGERROR("Failed to configure endpoint");
                            }
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
        }

        Status Controller::StartTunnel(grpc::ServerContext*, const google::protobuf::StringValue* request, Empty*)
        {
            return StartTunnel(request->value());
        } // StartTunnel

        Status Controller::StopTunnel(const std::string &name)
        {
            Status result(StatusCode::NOT_FOUND, "Unknown tunnel");
            auto builderIt  = tunnelBuilders.find(name);
            if(builderIt != tunnelBuilders.end())
            {
                builderIt->second->Shutdown();
                tunnelBuilders.erase(builderIt);
                result = Status();
                LOGINFO("Tunnel stopped");
            }
            return result;
        }

        Status Controller::StopTunnel(grpc::ServerContext*, const google::protobuf::StringValue* request, Empty*)
        {
            return StopTunnel(request->value());
        } // StopTunnel

        Status Controller::CompleteTunnel(grpc::ServerContext*, const remote::tunnels::CompleteTunnelRequest* request,
                                          remote::tunnels::CompleteTunnelResponse* response)
        {
            using namespace std;
            LOGTRACE("Called");
            Status result;

            LOGDEBUG("Waiting for keystore...");
            if(WaitForKeyStore())
            {

                LOGDEBUG("Keystore ready");
                auto builderIt = tunnelBuilders.find(request->tunnel().name());
                if(builderIt == tunnelBuilders.end())
                {
                    LOGTRACE("Creating tunnel builder");
                    auto newBuilder = make_shared<TunnelBuilder>(request->tunnel().encryptionmethod(), request->tunnel().remoteencryptedlistenaddress(),
                                      LoadServerCredentials(settings.credentials()),
                                      LoadChannelCredentials(settings.credentials()));
                    // tell the remote node how to connect to this node
                    response->set_encryptedconnectionuri(newBuilder->GetListenAddress());

                    // start this side
                    LOGTRACE("Configuring endpoint");
                    result = newBuilder->ConfigureEndpoint(request->tunnel().endnode(), keyFactoryChannel,
                                                           request->startkeystore(),
                                                           request->tunnel().keylifespan());

                    tunnelBuilders.emplace(request->tunnel().name(), move(newBuilder));
                    // Dont call StartTransfer because we are the slave node, the caller will use TransferBi
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
