/*!
* @file
* @brief SessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Interfaces/IPhotonGenerator.h"
#include "Algorithms/Util/WorkerThread.h"

namespace cqp
{
    namespace session
    {

        /**
         * @brief The AliceSessionController class cooirdinates the session
         */
        class CQPTOOLKIT_EXPORT AliceSessionController : public SessionController,
            protected WorkerThread
        {
        public:

            /**
             * @brief AliceSessionController
             * Constructor
             * @param creds credentials to use when contacting the peer controller
             * @param remotes A list of objects which need to know when the sessions start/stop
             * @param source The object which produces photons
             * @param theReportServer for passing on stats
             */
            AliceSessionController(std::shared_ptr<grpc::ChannelCredentials> creds,
                                   const RemoteCommsList& remotes,
                                   std::shared_ptr<IPhotonGenerator> source,
                                   std::shared_ptr<stats::ReportServer> theReportServer);

            ///@{
            /// @name ISessionController interface

            /** @copydoc ISessionController::StartSession
             * @details
             * @startuml
             * participant "AliceSessionController" as as
             * boundary "IPhotonGenerator" as ps
             * boundary "IDetector" as det
             *
             * [-> as : StartSession
             * activate as
             *     as -> as : base::StartSession()
             *     as -> as : Start()
             * deactivate as
             *
             * [-> as : DoWork()
             *     activate as
             *         loop
             *             as -> ps : StartFrame()
             *             as -> det : StartDetecting()
             *             as -> ps : Fire()
             *             as -> det : StopDetecting()
             *             as -> ps : EndFrame()
             *         end loop
             *     deactivate as
             * @enduml
             */
            grpc::Status StartSession(const remote::SessionDetailsFrom& sessionDetails) override;
            /// @copydoc ISessionController::EndSession
            void EndSession() override;

            /// @}

            ///@{
            /// @name remote::ISession interface

            /// @copydoc remote::ISession::SessionStarting
            /// @param context Connection details from the server
            grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty*) override;
            /// @copydoc remote::ISession::SessionEnding
            /// @param context Connection details from the server
            grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty*, google::protobuf::Empty*) override;
            ///@}

        protected: // methods
            void DoWork() override;

        protected: // members
            /// where photons are made
            std::shared_ptr<IPhotonGenerator> photonSource;
        }; // class AliceSessionController
    } // namespace session

} // namespace cqp


