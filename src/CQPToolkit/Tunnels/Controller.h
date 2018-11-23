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
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "QKDInterfaces/ITunnelServer.grpc.pb.h"
#include "CQPToolkit/Interfaces/IService.h"
#include "cryptopp/rng.h"
#include "cryptopp/seckey.h"
#include "QKDInterfaces/Tunnels.pb.h"
#include "CQPToolkit/Datatypes/Tunnels.h"
#include <condition_variable>

namespace grpc
{
    class ChannelCredentials;
    class ServerCredentials;
}

namespace cqp
{
    namespace tunnels
    {
        class TunnelBuilder;

        /**
         * @brief The Controller class
         * Handle the configuration of an end point from which tunnels are created
         */
        class CQPTOOLKIT_EXPORT Controller : public remote::tunnels::ITunnelServer::Service,
            public IServiceCallback
        {
        public:
            /**
             * @brief Controller
             * Constructor which does not detect devices, but uses the provided settings
             * @param initialSettings Configuration of the instance
             */
            explicit Controller(const remote::tunnels::ControllerDetails& initialSettings);

            ///@{
            /// @name ITunnelServer Implementation

            /// @copydoc remote::tunnels::ITunnelServer::GetSupportedSchemes
            /// @param context
            grpc::Status GetSupportedSchemes(grpc::ServerContext* context, const google::protobuf::Empty*, remote::tunnels::EncryptionSchemes* response) override;

            /// @copydoc remote::tunnels::ITunnelServer::GetControllerSettings
            /// @param context
            grpc::Status GetControllerSettings(grpc::ServerContext* context, const google::protobuf::Empty*, remote::tunnels::ControllerDetails* response) override;

            /// @copydoc remote::tunnels::ITunnelServer::ModifyTunnel
            /// @param context
            grpc::Status ModifyTunnel(grpc::ServerContext* context, const remote::tunnels::Tunnel* request, google::protobuf::Empty*) override;

            /// @copydoc remote::tunnels::ITunnelServer::DeleteTunnel
            /// @param context
            grpc::Status DeleteTunnel(grpc::ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty*) override;

            /// @copydoc remote::tunnels::ITunnelServer::StartTunnel
            /// @param context
            grpc::Status StartTunnel(grpc::ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty*) override;
            /** @copydoc remote::tunnels::ITunnelServer::StopTunnel
            * @param context
            * @details
            @startuml StartTunnelImplementation
                title StartTunnel Implementation
                hide footbox

                control "Controller : A" as ca
                participant "TunnelBuilder : A" as tba
                control "Controller : B" as cb
                participant "TunnelBuilder : B" as tbb

                [-> ca : StartTunnel
                activate ca
                    ca -> ca : WaitForKeyStore()
                    note over ca
                        This allows discovery to detect
                        the key store given a UID
                    end note

                    create tba
                    ca -> tba

                    ca -> ca : WaitForOtherController()
                    ca -> cb : CompleteTunnel(settings)
                    activate cb
                        cb -> cb : WaitForKeyStore()

                        create tbb
                        cb -> tbb
                        cb -> tbb : ConfigureEndpoint(settings)

                        ca <-- cb : transferURI
                    deactivate cb
                    ca -> tba : ConfigureEndpoint(transferURI)

                deactivate ca

            @enduml
            */
            grpc::Status StopTunnel(grpc::ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty*) override;

            /// @copydoc remote::tunnels::ITunnelServer::CompleteTunnel
            grpc::Status CompleteTunnel(grpc::ServerContext*, const remote::tunnels::CompleteTunnelRequest* request, remote::tunnels::CompleteTunnelResponse* response) override;
            ///@}

            /// Default destructor
            virtual ~Controller() override;

            /**
             * @brief StartAllTunnels
             */
            void StartAllTunnels();

            /**
             * @brief CloseAllTunnels
             * Shut down any open tunnels
             */
            void StopAllTunnels();

            /**
             * @brief GetControllerSettings
             * @param response
             */
            void GetControllerSettings(remote::tunnels::ControllerDetails& response);

            // IServiceCallback interface
            void OnServiceDetected(const RemoteHosts& newServices, const RemoteHosts& deletedServices) override;
        protected: // methods
            /**
             * @brief WaitForKeyStore
             * @param timeout
             * @return true if keystore is available
             */
            bool WaitForKeyStore(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

            /**
             * @brief FindController
             * Create a channel to for a tunnel
             * @param tun
             * @return The channel
             */
            std::shared_ptr<grpc::Channel> FindController(const remote::tunnels::Tunnel& tun);
        protected: // members

            /// The current settings for this controller
            remote::tunnels::ControllerDetails settings;
            /// Prevent corruption of settings
            std::mutex settingsMutex;

            /// credentials to use to connect to peers
            std::shared_ptr<grpc::ChannelCredentials> clientCreds;
            /// credentials to use to start server
            std::shared_ptr<grpc::ServerCredentials> serverCreds;

            /// a list of channels for different tunnel controllers
            using ControllerList = std::unordered_map<std::string, std::shared_ptr<grpc::Channel>>;
            /// Other known controllers which can be contacted
            ControllerList endpointsByName;
            /// controllers indexed by id
            ControllerList endpointsByID;
            /// access control for known controllers
            std::mutex controllerDetectedMutex;
            /// for waiting for change to know controllers
            std::condition_variable controllerDetectedCv;
            /// a list of builders
            using TunnelBuilderList = std::unordered_map<std::string, std::shared_ptr<TunnelBuilder>>;
            /// tunnels currently being managed
            TunnelBuilderList tunnelBuilders;
            /// the location of our keystore
            std::string keyStoreFactoryUri;
            /// connection to our keystore
            std::shared_ptr<grpc::Channel> keyFactoryChannel;

        }; // class Controller
    } // namespace tunnels
} // namespace cqp
