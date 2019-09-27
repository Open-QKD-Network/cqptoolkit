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
#include "QKDInterfaces/ISync.grpc.pb.h"
#include "IDQDevices/idqdevices_export.h"
#include <thread>

namespace cqp
{

    class IDQDEVICES_EXPORT Clavis3Session : public cqp::session::SessionController,
        public remote::ISync::Service
    {
    public:
        Clavis3Session(const std::string& hostname,
                       std::shared_ptr<grpc::ChannelCredentials> newCreds,
                       std::shared_ptr<stats::ReportServer> theReportServer,
                       bool disableControl = false, const std::string& keyFile = "");

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


        grpc::Status ReleaseKeys(grpc::ServerContext* ctx, const remote::IdList* request, google::protobuf::Empty*) override;
        grpc::Status SendInitialKey(grpc::ServerContext* ctx, const google::protobuf::Empty*, google::protobuf::Empty*) override;

        remote::Side::Type GetSide() const;

        bool Initialise(const remote::SessionDetails& sessionDetails);
        void SetInitialKey(std::unique_ptr<PSK> initailKey);
        KeyPublisher* GetKeyPublisher();

        bool SystemAvailable();
    protected: // methods

        void PassOnKeys();
    protected: // members
        class Impl;

        using ClavisKeyList = std::vector<std::pair<UUID, PSK>>;

        std::unique_ptr<Impl> pImpl;
        std::unique_ptr<KeyPublisher> keyPub;
        std::thread keyReader;
        std::atomic_bool keepReadingKeys{true};
        bool controlsEnabled = true;
        std::map<UUID, PSK> bufferedKeys;
        std::mutex bufferedKeysMutex;
        std::condition_variable bufferedKeysCv;
    };

} // namespace cqp

#endif // HAVE_IDQ4P
