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
#include "Algorithms/Util/Provider.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Interfaces/IRemoteComms.h"
#include <condition_variable>
#include "Algorithms/Util/Strings.h"

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{
    // pre-declarations for limiting build complexity
    class IRandom;

    namespace net
    {
        class TwoWayServerConnector;
    }

    namespace stats
    {
        class ReportServer;
    }

    namespace session
    {

        /**
         * @brief The SessionController class
         */
        class CQPTOOLKIT_EXPORT SessionController :
            public remote::ISession::Service,
            public virtual ISessionController
        {
        public:
            /// A list of connectable objects
            using RemoteCommsList = std::vector<std::shared_ptr<IRemoteComms>>;
            /// A list of services to register with builder
            using Services = std::vector<grpc::Service*>;

            /**
             * @brief SessionController
             * Constructor
             * @param creds credentials to use when contacting the peer controller
             * @param services A list of services to register with builder
             * @param remotes A list of objects which need to know when the sessions start/stop
             */
            SessionController(std::shared_ptr<grpc::ChannelCredentials> creds,
                              const Services& services,
                              const RemoteCommsList& remotes,
                              std::shared_ptr<stats::ReportServer> theReportServer);

            /// Destructor
            ~SessionController() override;

            ///@{
            /// @name ISessionController interface

            /// @copydoc ISessionController::GetConnectionAddress
            std::string GetConnectionAddress() const override
            {
                return myAddress + ":" + std::to_string(listenPort);
            }

            /// @copydoc ISessionController::StartSession
            grpc::Status StartSession(const remote::SessionDetails& sessionDetails) override;

            /// @copydoc ISessionController::EndSession
            void EndSession() override;

            /// @copydoc ISessionController::StartServer
            grpc::Status StartServer(const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;
            /// @copydoc ISessionController::StartServerAndConnect
            grpc::Status StartServerAndConnect(URI otherController, const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;

            /// @copydoc ISessionController::StartServerAndConnect
            grpc::Status Connect(URI otherController) override;

            /// @copydoc ISessionController::GetLinkStatus
            grpc::Status GetLinkStatus(grpc::ServerContext* context, ::grpc::ServerWriter<remote::LinkStatus>* writer) override;
            ///@}

            ///@{
            /// @name remote::ISession

            /// @copydoc remote::ISession::SessionStarting
            /// @param context Connection details from the server
            grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty*) override;
            /// @copydoc remote::ISession::SessionEnding
            /// @param context Connection details from the server
            grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty*, google::protobuf::Empty*) override;
            ///@}

            struct PropertyNames
            {
                static CONSTSTRING sessionActive = "sessionActive";
                static CONSTSTRING from = "from";
                static CONSTSTRING to = "to";
            };

        protected: // methods
            void UpdateStatus(remote::LinkStatus::State newState, int errorCode = grpc::StatusCode::OK);

        protected: // members

            /// the address to connect to us
            std::string myAddress;
            /// our server, other interfaces will hang off of this
            std::unique_ptr<grpc::Server> server;
            /// The address of the controller on the other side
            std::string pairedControllerUri;

            /// The port used to connect
            int listenPort = 0;
            /// To setup two way connections
            net::TwoWayServerConnector* twoWayComm = nullptr;
            /// A list of services to register with builder
            Services services;
            /// A list of objects which need to know when the sessions start/stop
            RemoteCommsList remoteComms;
            /// mutex for waiting for session end
            std::mutex threadControlMutex;
            /// cv for waiting for end of session
            std::condition_variable linkStatusCv;
            /// track the session state
            remote::LinkStatus sessionState;
            /// Collects data from all the stat producers
            std::shared_ptr<stats::ReportServer> reportServer;
        }; // class SessionController

    } // namespace session
} // namespace cqp


