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
#include "CQPToolkit/Util/Provider.h"
#include <grpc++/channel.h>
#include <thread>

namespace cqp
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
        ClavisController(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds);

        ///@{
        /// @name ISessionController

        /// @copydoc cqp::ISessionController::StartSession
        grpc::Status StartSession(const remote::OpticalParameters& params) override;

        /// @copydoc cqp::ISessionController::EndSession
        void EndSession() override;

        /// @copydoc cqp::ISessionController::GetKeyPublisher
        IKeyPublisher* GetKeyPublisher() override;

        /// @copydoc cqp::ISessionController::GetStats
        std::vector<stats::StatCollection*> GetStats() const override;
        ///@}

        ///@{
        /// @name ISession interface

        /// @copydoc cqp::remote::ISession::SessionStarting
        /// @param context Connection details from the server
        grpc::Status SessionStarting(grpc::ServerContext* context,
                                     const remote::SessionDetails* request,
                                     google::protobuf::Empty*) override;

        /// @copydoc cqp::remote::ISession::SessionEnding
        /// @param context Connection details from the server
        grpc::Status SessionEnding(
            grpc::ServerContext* context,
            const google::protobuf::Empty*,
            google::protobuf::Empty*) override;
        /// @}

        /// @copydoc SessionController::RegisterServices
        void RegisterServices(grpc::ServerBuilder& builder) override;

        /**
         * @brief GetSide
         * @return Which type of device is it
         */
        remote::Side::Type GetSide();
    protected:

        /**
         * @brief ReadKey
         * Pull key from the wrapper and pass it on to the keystore
         * @param reader source of key from the wrapper
         * @param wrapperCtx connection context
         */
        void ReadKey(std::unique_ptr<grpc::ClientReader<remote::SharedKey> > reader, std::unique_ptr<grpc::ClientContext> wrapperCtx);

        /// whether the threads should exit
        bool keepGoing = true;
        /// run the ReadKey call
        std::thread readThread;
        /// Setting provided by the wrapper
        remote::WrapperDetails myWrapperDetails;
        /// channel to wrapper
        std::shared_ptr<grpc::Channel> channel;
        /// wrapper stub
        std::unique_ptr<remote::IIDQWrapper::Stub> wrapper;
    };// ClavisController

} // namespace cqp


