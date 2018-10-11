/*!
* @file
* @brief SessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/ISession.grpc.pb.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "CQPToolkit/Util/Provider.h"

namespace cqp
{
    // pre-declarations for limiting build complexity
    class PublicKeyService;
    class IRandom;

    namespace net
    {
        class TwoWayServerConnector;
    }

    /**
     * @brief The SessionController class
     */
    class SessionController :
        public remote::ISession::Service,
        public virtual ISessionController
    {
    public:
        ///@{
        /// @name ISessionController interface

        /// @copydoc ISessionController::GetConnectionAddress
        std::string GetConnectionAddress() const override
        {
            return myAddress + ":" + std::to_string(listenPort);
        }

        /// @copydoc ISessionController::StartSession
        grpc::Status StartSession(const remote::OpticalParameters& params) override;

        /// @copydoc ISessionController::EndSession
        void EndSession() override;

        /// @copydoc ISessionController::StartServer
        grpc::Status StartServer(const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;
        /// @copydoc ISessionController::StartServerAndConnect
        grpc::Status StartServerAndConnect(URI otherController, const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;
        ///@}

        ///@{
        /// @name remote::ISession

        /// @copydoc remote::ISession::SessionStarting
        /// @param context Connection details from the server
        grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetails* request, google::protobuf::Empty*) override;
        /// @copydoc remote::ISession::SessionEnding
        /// @param context Connection details from the server
        grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty*, google::protobuf::Empty*) override;
        ///@}

    protected: // members

        /// Our authenticatio token for getting shared secrets
        std::string keyToken;
        /// the address to connect to us
        std::string myAddress;
        /// our server, other interfaces will hang off of this
        std::unique_ptr<grpc::Server> server;
        /// Exchange keys with other sites
        PublicKeyService* pubKeyServ = nullptr;
        /// The address of the controller on the other side
        std::string pairedControllerUri;
        /// a random number generator
        IRandom* rng = nullptr;

        /// The port used to connect
        int listenPort = 0;
        /// To setup two way connections
        net::TwoWayServerConnector* twoWayComm = nullptr;

    protected: // methods

        /**
         * @brief SessionController
         * Constructor
         * @param creds credentials to use when contacting the peer controller
         */
        SessionController(std::shared_ptr<grpc::ChannelCredentials> creds);

        /// Distructor
        ~SessionController() override;

        /**
         * @brief RegisterServices
         * register all the classes which provide a remote interface
         * @param builder
         */
        virtual void RegisterServices(grpc::ServerBuilder& builder) = 0;

    }; // class SessionController

} // namespace cqp


