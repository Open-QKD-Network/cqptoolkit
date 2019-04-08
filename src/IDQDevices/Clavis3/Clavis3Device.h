/*!
* @file
* @brief Clavis3Device
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 14/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(HAVE_IDQ4P)

#include <string>
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "IDQDevices/idqdevices_export.h"
#include "CQPToolkit/Session/SessionController.h"
#include "QKDInterfaces/Site.pb.h"

namespace cqp
{

    class IDQDEVICES_EXPORT Clavis3Device :
        public virtual IQKDDevice,
        public session::SessionController,
        public KeyPublisher
    {
    public:
        Clavis3Device(const std::string& hostname, remote::Side::Type theSide, std::shared_ptr<grpc::ChannelCredentials> creds, std::shared_ptr<stats::ReportServer> theReportServer);
        ~Clavis3Device() override;

        ///@{
        /// @name IQKDDevice interface

        /// @copydoc IQKDDevice::GetDriverName
        std::string GetDriverName() const override;
        /// @copydoc IQKDDevice::GetAddress
        URI GetAddress() const override;
        /// @copydoc IQKDDevice::Initialise
        bool Initialise(const remote::SessionDetails& sessionDetails) override;
        /// @copydoc IQKDDevice::GetSessionController
        ISessionController* GetSessionController() override;
        /// @copydoc IQKDDevice::GetKeyPublisher
        KeyPublisher* GetKeyPublisher() override;
        /// @copydoc IQKDDevice::GetDeviceDetails
        remote::DeviceConfig GetDeviceDetails() override;
        /// @copydoc IQKDDevice::SetInitialKey
        void SetInitialKey(std::unique_ptr<PSK> initailKey) override;
        /// @copydoc IQKDDevice::RegisterServices
        void RegisterServices(grpc::ServerBuilder& builder) override;
        ///@}

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
    protected: // methods
        void PassOnKeys();
    protected: // members
        class Impl;
        std::unique_ptr<Impl> pImpl;
        remote::DeviceConfig deviceConfig;
    };

} // namespace cqp

#endif // HAVE_IDQ4P
