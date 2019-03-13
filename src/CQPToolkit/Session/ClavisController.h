/*!
* @file
* @brief ClavisController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <QKDInterfaces/ISession.grpc.pb.h>
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Session/PublicKeyService.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "QKDInterfaces/IIDQWrapper.grpc.pb.h"
#include "Algorithms/Util/Provider.h"
#include <grpc++/channel.h>
#include <thread>
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{
    namespace session
    {
        /**
         * @brief The ClavisController class
         * Session controller for Clavis devices
         */
        class CQPTOOLKIT_EXPORT ClavisController :
                public SessionController, public Provider<IKeyCallback>
        {
        public:
            /**
             * @brief ClavisController
             * @param address The uri of the wrapper
             * @param creds credentials to use when connecting to the wrapper
             */
            ClavisController(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds,
                             std::shared_ptr<stats::ReportServer> theReportServer);

            /// distructor
            ~ClavisController() override;

            ///@{
            /// @name ISessionController

            /// @copydoc cqp::ISessionController::StartSession
            grpc::Status StartSession(const remote::SessionDetails& sessionDetails) override;

            /// @copydoc cqp::ISessionController::EndSession
            void EndSession() override;

            ///@}

            ///@{
            /// @name ISession interface

            /// @copydoc cqp::remote::ISession::SessionStarting
            /// @param context Connection details from the server
            grpc::Status SessionStarting(grpc::ServerContext* context,
                                         const remote::SessionDetailsFrom* request,
                                         google::protobuf::Empty*) override;

            /// @copydoc cqp::remote::ISession::SessionEnding
            /// @param context Connection details from the server
            grpc::Status SessionEnding(
                    grpc::ServerContext* context,
                    const google::protobuf::Empty*,
                    google::protobuf::Empty*) override;

            /// @}

            /**
             * @brief GetSide
             * @return Which type of device is it
             */
            remote::Side::Type GetSide();

            bool Initialise(const remote::SessionDetails& session);
        protected:

            /**
             * @brief ReadKey
             * Pull key from the wrapper and pass it on to the keystore
             * @param reader source of key from the wrapper
             * @param wrapperCtx connection context
             */
            void ReadKey(std::unique_ptr<grpc::ClientReader<remote::SharedKey> > reader, std::unique_ptr<grpc::ClientContext> wrapperCtx);

            /**
             * @brief CollectStats
             * Read stats from the external process
             */
            void CollectStats();

            /// whether the threads should exit
            bool keepGoing = true;
            /// run the ReadKey call
            std::thread readThread;
            /// Setting provided by the wrapper
            remote::WrapperDetails myWrapperDetails;
            /// gets the stats from the device
            std::thread statsThread;
            /// channel to wrapper
            std::shared_ptr<grpc::Channel> channel;
            /// wrapper stub
            std::unique_ptr<remote::IIDQWrapper::Stub> wrapper;
            /// Exchange keys with other sites
            PublicKeyService* pubKeyServ = nullptr;
            /// Our authentication token for getting shared secrets
            std::string keyToken;
            remote::SessionDetails sessionDetails;
        };// ClavisController
    }

} // namespace cqp


