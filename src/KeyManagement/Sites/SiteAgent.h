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
#pragma once
#include "KeyManagement/keymanagement_export.h"
#include "QKDInterfaces/INetworkManager.grpc.pb.h"
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "Algorithms/Datatypes/Keys.h"
#include <grpc++/server.h>
#include <unordered_map>
#include <thread>
#include "QKDInterfaces/IDevice.grpc.pb.h"

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{
    class IKeyCallback;
    // pre-declarations for limiting build complexity
    namespace keygen
    {
        class KeyStore;
        class KeyStoreFactory;
        class IBackingStore;
    }

    namespace stats
    {
        class ReportServer;
    }

    namespace net
    {
        class ServiceDiscovery;
    }

    namespace stats
    {
        class StatisticsLogger;
    }

    /**
     * @brief The SiteAgent class
     */
    class KEYMANAGEMENT_EXPORT SiteAgent : public remote::ISiteAgent::Service
    {
    public:

        /**
         * @brief SiteAgent
         * @param config
         */
        explicit SiteAgent(const remote::SiteAgentConfig& config);

        /// Destructor
        ~SiteAgent() override;

        /**
         * @brief RegisterWithDiscovery
         * Pass details to the service discovery so that this site can be discovered
         * automagically
         * @param sd
         * @return true if it was successfully registered
         */
        bool RegisterWithDiscovery(net::ServiceDiscovery& sd);

        /**
         * @brief GetConnectionAddress
         * @return The address to use to connect to this site
         */
        const std::string& GetConnectionAddress() const
        {
            return myConfig.connectionaddress();
        }

        /**
         * @brief GetKeyStoreFactory
         * @return The keystore factory for this site
         */
        std::shared_ptr<keygen::KeyStoreFactory> GetKeyStoreFactory()
        {
            return keystoreFactory;
        }

        ///@{
        /// @name ISiteAgent interface

        /** @copydoc remote::ISiteAgent::StartNode
         * @param context Connection details from the server
         * @details
         * @startuml
         * [-> SiteAgent : StartNode
         * activate SiteAgent
         *     alt loop for each hop
         *         note over SiteAgent
         *             A hop is a pair of addresses, A and B.
         *             A multi-hop link would coontain [A, B], [B, C]
         *         end note
         *         alt If First address is this site
         *             SiteAgent -> SiteAgent : StartLeftSide()
         *         else If second address is this site
         *             SiteAgent -> SiteAgent : StartRightSide()
         *         end alt
         *     end alt
         *
         *     alt If we're at eather end of the link
         *         SiteAgent -> KeyStoreFactory : GetKeyStore()
         *         SiteAgent -> KeyStore : SetPath()
         *     end alt
         * deactivate SiteAgent
         * @enduml
         */
        grpc::Status StartNode(grpc::ServerContext* context, const remote::PhysicalPath* request, google::protobuf::Empty*) override;

        /// @copydoc remote::ISiteAgent::EndKeyExchange
        /// @param context Connection details from the server
        grpc::Status EndKeyExchange(grpc::ServerContext* context, const remote::PhysicalPath* request, google::protobuf::Empty*) override;

        /// @copydoc remote::ISiteAgent::GetSiteDetails
        /// @param context Connection details from the server
        grpc::Status GetSiteDetails(grpc::ServerContext* context, const google::protobuf::Empty*, remote::Site* response) override;

        /// @copydoc remote::ISiteAgent::RegisterDevice
        /// @param context Connection details from the server
        grpc::Status RegisterDevice(grpc::ServerContext* context, const remote::ControlDetails* request, google::protobuf::Empty* response) override;
        /// @copydoc remote::ISiteAgent::UnregisterDevice
        /// @param context Connection details from the server
        grpc::Status UnregisterDevice(grpc::ServerContext* context, const remote::DeviceID* request, google::protobuf::Empty* response) override;
        ///@}

        /// Start any devices with static links
        void ConnectStaticLinks();

    protected: // members

        /// our server
        std::unique_ptr<grpc::Server> server;
        /// Stores the state of the site
        struct SiteState
        {
            /// The Channel for the site
            std::shared_ptr<grpc::Channel> channel;
            /// The link status
            remote::LinkStatus_State state = remote::LinkStatus_State::LinkStatus_State_Inactive;
        };

        /// communication channels to other site agents for creating stubs
        std::unordered_map<std::string, SiteState> otherSites;
        /// configuration settings
        remote::SiteAgentConfig myConfig;
        /// Holds all keystores created at this site
        std::shared_ptr<keygen::KeyStoreFactory> keystoreFactory;
        /// metadata tag name for pass a session controller address
        const char* sessionAddress = "sessionaddress";

        struct DeviceConnection
        {
            std::shared_ptr<grpc::Channel> channel;
            grpc::ClientContext context;
            std::shared_ptr<keygen::KeyStore> keySink;
            std::thread readerThread;
            std::thread statsThread;

            ~DeviceConnection()
            {
                if(readerThread.joinable())
                {
                    readerThread.join();
                }
                if(statsThread.joinable())
                {
                    statsThread.join();
                }
            }

            void ReadStats(stats::ReportServer* reportServer);
        };

        /// a list of devices being actively used
        std::unordered_map<std::string, std::shared_ptr<DeviceConnection>> devicesInUse;
        /// collects statistics reports and publishes them to clients
        std::unique_ptr<stats::ReportServer> reportServer;
        /// configuration for this site
        remote::Site siteDetails;
        std::mutex siteDetailsMutex;
        std::condition_variable siteDetailsChanged;
        std::thread netManRegister;

        /// callback type for status updates
        using StatusCallback = std::function<void (const cqp::remote::LinkStatus&)>;
        /// all registered callbacks
        std::unordered_map<uint64_t, StatusCallback> statusCallbacks;
        /// constrols access to the callback list
        std::mutex statusCallbackMutex;
        /// counter for giving caller a unique id
        uint64_t statusCounter = 0;
        std::atomic_bool shutdown {false};

    protected: // methods
        /**
         * @brief GetSiteChannel
         * records know sites and creates a channel to them
         * @param connectionAddress The address of the other site agent
         * @return A channel to that agent
         */
        std::shared_ptr<grpc::Channel> GetSiteChannel(const std::string& connectionAddress);
        /**
         * @brief StartLeftSide
         * Start the node
         * @param path The entire path being started
         * @param hopPair the hop being started
         * @return Success
         * @details
         * @startuml
         * participant SiteAgent
         * boundary ISiteAgent
         *
         * [-> SiteAgent : StartLeftSide
         * activate SiteAgent
         *
         *     alt if not connected
         *         SiteAgent -> SiteAgent : PrepHop()
         *     end alt
         *
         *     SiteAgent -> ISiteAgent : StartNode()
         *
         * deactivate SiteAgent
         * @enduml
         */
        grpc::Status ForwardOnStartNode(const remote::PhysicalPath* path, const std::string& secondSite, const std::string& localSessionAddress);
        /**
         * @brief StartRightSide
         * Start the node
         * @param ctx server context for passing metadata
         * @param hopPair The hop being started
         * @param remoteSessionAddress The address of the other sides session controller service
         * @return Success
         * @details
         * @startuml
         * participant SiteAgent
         * control ISessionController as sc
         *
         * [-> SiteAgent : StartRightSide
         * activate SiteAgent
         *
         *     alt if not connected
         *         SiteAgent -> SiteAgent : PrepHop()
         *         SiteAgent -> sc : StartServerAndConnect()
         *         SiteAgent -> sc : StartSession()
         *     end alt
         *
         * deactivate SiteAgent
         * @enduml
         */
        grpc::Status StartSession(std::shared_ptr<grpc::Channel> channel, const remote::SessionDetails& sessionDetails, const std::string& remoteSessionAddress);
        /**
         * @brief PrepHop
         * Setup the devices/controllers/etc
         * @param deviceId Which device to use for this hop
         * @param destination The peer to connect to
         * @param[out] controller The controller to be used to complete the hop setup
         * @return Success
         */
        grpc::Status PrepHop(const std::string& deviceId, const std::string& destination, std::string& localSessionAddress, remote::SessionDetails& params);

        /**
         * @brief StopNode
         * Stop generating key with the specified device
         * @param deviceUri which device to stop
         * @return status of command
         */
        grpc::Status StopNode(const std::string& deviceUri);

        /**
         * @brief AddressIsThisSite
         * @param address
         * @return true if address resolves to this site agents address
         */
        bool AddressIsThisSite(const std::string& address);

        /**
         * @brief RegisterWithNetMan
         * Register our service with the network manager.
         * Blocks until shutdown, triggering the siteDetailsChanged cv will trigger a re-registering
         */
        void RegisterWithNetMan(std::string netManUri, std::shared_ptr<grpc::ChannelCredentials> creds);

        static void ProcessKeys(std::shared_ptr<DeviceConnection> connection, std::unique_ptr<PSK> initialKey);

        grpc::Status StartNode(grpc::ServerContext* ctx, remote::HopPair& hop, remote::PhysicalPath& myPath);

    };

}
