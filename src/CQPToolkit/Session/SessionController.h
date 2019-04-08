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
    class Channel;
}

namespace cqp
{
    // pre-declarations for limiting build complexity
    class IRandom;

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

            /**
             * @brief SessionController
             * Constructor
             * @param creds credentials to use when contacting the peer controller
             * @param theReportServer for publishing stats
             * @param remotes A list of objects which need to know when the sessions start/stop
             */
            SessionController(std::shared_ptr<grpc::ChannelCredentials> creds,
                              const RemoteCommsList& remotes,
                              std::shared_ptr<stats::ReportServer> theReportServer);

            /// Destructor
            ~SessionController() override;

            ///@{
            /// @name ISessionController interface

            /// @copydoc ISessionController::StartSession
            grpc::Status StartSession(const remote::SessionDetailsFrom& sessionDetails) override;

            /// @copydoc ISessionController::EndSession
            void EndSession() override;

            /// @copydoc ISessionController::RegisterServices
            void RegisterServices(grpc::ServerBuilder &builder) override;

            /// @copydoc ISessionController::Connect
            grpc::Status Connect(URI otherController) override;

            /// @copydoc ISessionController::Disconnect
            void Disconnect() override;

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

            /// names for key-pair properties
            struct PropertyNames
            {
                /// whether the session is running
                static CONSTSTRING sessionActive = "sessionActive";
                /// source
                static CONSTSTRING from = "from";
                /// destination
                static CONSTSTRING to = "to";
            };

        protected: // methods
            /**
             * @brief UpdateStatus
             * Send an update to the status
             * @param newState
             * @param errorCode
             */
            void UpdateStatus(remote::LinkStatus::State newState, int errorCode = grpc::StatusCode::OK);

        protected: // members

            /// The connection to the controller on the other side
            std::shared_ptr<grpc::Channel> otherControllerChannel;
            /// The address of the controller on the other side
            std::string pairedControllerUri;
            /// connection credentials
            std::shared_ptr<grpc::ChannelCredentials> creds;

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


