/*!
* @file
* @brief Clavis3Session
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28/6/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(HAVE_IDQ4P)
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"

namespace cqp
{

    class IDQDEVICES_EXPORT Clavis3Session : public cqp::session::SessionController
    {
    public:
        Clavis3Session(const std::string& hostname,
                       std::shared_ptr<grpc::ChannelCredentials> newCreds,
                       std::shared_ptr<stats::ReportServer> theReportServer);

        ~Clavis3Session() override;

        ///@{
        /// @name ISessionController interface

        /// @copydoc ISessionController::StartSession
        grpc::Status StartSession(const remote::SessionDetailsFrom& sessionDetails) override;
        /// @copydoc ISessionController::EndSession
        void EndSession() override;
        ///@}

        ///@{
        /// @name ISession interface

        /// @copydoc remote::ISession::SessionStarting
        grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetailsFrom* request, google::protobuf::Empty*) override;
        /// @copydoc remote::ISession::SessionEnding
        grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty*) override;
        ///@}

        remote::Side::Type GetSide() const;

        bool Initialise(const remote::SessionDetails& sessionDetails);
        void SetInitialKey(std::unique_ptr<PSK> initailKey);
        KeyPublisher* GetKeyPublisher();

    protected: // methods

        void PassOnKeys();
    protected: // members
        class Impl;
        std::unique_ptr<Impl> pImpl;
        std::unique_ptr<KeyPublisher> keyPub;
    };

} // namespace cqp

#endif // HAVE_IDQ4P
