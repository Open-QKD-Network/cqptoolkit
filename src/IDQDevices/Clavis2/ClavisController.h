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
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "IDQDevices/IIDQWrapper.grpc.pb.h"
#include "Algorithms/Util/Provider.h"
#include <grpc++/channel.h>
#include <thread>
#include "QKDInterfaces/Device.pb.h"
#include "IDQDevices/idqdevices_export.h"

namespace cqp
{
    class Clavis;
    class IDQSequenceLauncher;

    namespace session
    {
        /**
         * @brief The ClavisController class
         * Session controller for Clavis devices
         */
        class IDQDEVICES_EXPORT ClavisController :
            public SessionController, public Provider<IKeyCallback>,
            public remote::IIDQWrapper::Service
        {
        public:
            /**
             * @brief ClavisController
             * @param address The uri of the wrapper
             * @param creds credentials to use when connecting to the wrapper
             */
            ClavisController(std::shared_ptr<grpc::ChannelCredentials> creds,
                             std::shared_ptr<stats::ReportServer> theReportServer);

            /// distructor
            ~ClavisController() override;

            ///@{
            /// @name ISessionController

            /// @copydoc cqp::ISessionController::StartSession
            grpc::Status StartSession(const remote::SessionDetailsFrom& sessionDetails) override;

            /// @copydoc cqp::ISessionController::EndSession
            void EndSession() override;

            ///@}

            ///@{
            /// @name ISession interface

            void RegisterServices(grpc::ServerBuilder &builder) override;

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

            ///@{
            /// @name IIDQWrapper interface
            grpc::Status UseKeyID(grpc::ServerContext* context, ::grpc::ServerReader<remote::KeyIdValueList>* reader, google::protobuf::Empty* response) override;
            ///@}

            /**
             * @brief GetSide
             * @return Which type of device is it
             */
            remote::Side::Type GetSide();

            bool Initialise(std::unique_ptr<PSK> initialKey);
        protected:

            /**
             * @brief ReadKey
             * Pull key from the wrapper and pass it on to the keystore
             * @param reader source of key from the wrapper
             * @param wrapperCtx connection context
             */
            void ReadKey();

            std::unique_ptr<cqp::Clavis> device;
            std::unique_ptr<IDQSequenceLauncher> launcher;

            /// whether the threads should exit
            bool keepGoing = true;
            /// run the ReadKey call
            std::thread readThread;
            /// gets the stats from the device
            std::thread statsThread;
            /// Our authentication token for getting shared secrets
            std::unique_ptr<PSK> authKey;
            remote::Side::Type side;
        };// ClavisController
    }

} // namespace cqp


