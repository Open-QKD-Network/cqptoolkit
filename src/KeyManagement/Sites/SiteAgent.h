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
#include "QKDInterfaces/ISiteDetails.grpc.pb.h"
#include "Algorithms/Datatypes/Keys.h"
#include <grpc++/server.h>
#include <unordered_map>

namespace cqp
{
    // pre-declarations for limiting build complexity
    namespace keygen
    {
        class KeyStoreFactory;
        class IBackingStore;
    }
    class DeviceFactory;
    class IQKDDevice;
    class ISessionController;
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
    class KEYMANAGEMENT_EXPORT SiteAgent : public remote::ISiteAgent::Service,
        public remote::ISiteDetails::Service
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

        /// @copydoc remote::ISiteAgent::StartNode
        /// @param context Connection details from the server
        grpc::Status StartNode(grpc::ServerContext* context, const remote::PhysicalPath* request, google::protobuf::Empty*) override;

        /// @copydoc remote::ISiteAgent::EndKeyExchange
        /// @param context Connection details from the server
        grpc::Status EndKeyExchange(grpc::ServerContext* context, const remote::PhysicalPath* request, google::protobuf::Empty*) override;

        /// @copydoc remote::ISiteAgent::GetSiteDetails
        /// @param context Connection details from the server
        grpc::Status GetSiteDetails(grpc::ServerContext* context, const google::protobuf::Empty*, remote::Site* response) override;

        ///@}

        ///@{
        /// @name ISiteDetails interface

        grpc::Status GetLinkStatus(grpc::ServerContext* context, const google::protobuf::Empty* request, ::grpc::ServerWriter<remote::LinkStatus>* writer) override;
        ///@}

        /// Start any devices with static links
        void ConnectStaticLinks();

        /// Register our service with the network manager
        void RegisterWithNetMan();

    protected: // members

        /// our server
        std::unique_ptr<grpc::Server> server;
        /// channel to the network manager
        std::shared_ptr<grpc::Channel> netmanChannel;
        /// Stores the state of the site
        struct SiteState
        {
            std::shared_ptr<grpc::Channel> channel;
            remote::LinkStatus_State state = remote::LinkStatus_State::LinkStatus_State_Inactive;
        };

        /// communication channels to other site agents for creating stubs
        std::unordered_map<std::string, SiteState> otherSites;
        /// configuration settings
        remote::SiteAgentConfig myConfig;
        /// The network manager to register with
        std::unique_ptr<remote::INetworkManager::Stub> netMan = nullptr;
        /// Holds all keystores created at this site
        std::shared_ptr<keygen::KeyStoreFactory> keystoreFactory;
        /// Holds all the devices which this site controls
        std::unique_ptr<DeviceFactory> deviceFactory;
        /// metadata tag name for pass a session controller address
        const char* sessionAddress = "sessionaddress";
        /// a list of devices being actively used
        std::unordered_map<std::string, std::shared_ptr<IQKDDevice>> devicesInUse;
        /// collects statistics reports and publishes them to clients
        std::unique_ptr<stats::ReportServer> reportServer;
        /// configuration for this site
        remote::Site siteDetails;

        using StatusCallback = std::function<void (const cqp::remote::LinkStatus&)>;
        std::unordered_map<uint64_t, StatusCallback> statusCallbacks;
        std::mutex statusCallbackMutex;
        uint64_t statusCounter = 0;
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
         */
        grpc::Status StartLeftSide(const remote::PhysicalPath* path, const remote::HopPair& hopPair);
        /**
         * @brief StartRightSide
         * Start the node
         * @param ctx server context for passing metadata
         * @param hopPair The hop being started
         * @param remoteSessionAddress The address of the other sides session controller service
         * @return Success
         */
        grpc::Status StartRightSide(grpc::ServerContext* ctx, const remote::HopPair& hopPair, const std::string& remoteSessionAddress);
        /**
         * @brief PrepHop
         * Setup the devices/controllers/etc
         * @param deviceId Which device to use for this hop
         * @param destination The peer to connect to
         * @param[out] controller The controller to be used to complete the hop setup
         * @return Success
         */
        grpc::Status PrepHop(const std::string& deviceId, const std::string& destination, ISessionController*& controller);

        /**
         * @brief StopNode
         * Stop generating key with the specified device
         * @param deviceUri which device to stop
         * @return status of command
         */
        grpc::Status StopNode(const std::string& deviceUri);

        /**
         * @brief SendStatusUpdate
         * Trigger a status update to the listeners
         * @param destination
         * @param state
         * @param status
         */
        void SendStatusUpdate(const std::string& destination, remote::LinkStatus_State state, grpc::Status status = grpc::Status());

        /**
         * @brief AddressIsThisSite
         * @param address
         * @return true if address resolves to this site agents address
         */
        bool AddressIsThisSite(const std::string& address);
    };

}
