/*!
* @file
* @brief RemoteQKDDevice
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/2/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/IDevice.grpc.pb.h"
#include <grpc++/security/server_credentials.h>
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include <condition_variable>
#include <mutex>
#include <vector>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp {

    class IQKDDevice;
    namespace remote {
        class DeviceConfig;
    }

    class CQPTOOLKIT_EXPORT RemoteQKDDevice :
            public virtual IKeyCallback,
            public remote::IDevice::Service
    {
    public:
        RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                        std::shared_ptr<grpc::ServerCredentials> creds = grpc::InsecureServerCredentials());

        ///@{
        /// @name remote::IDevice interface

        /// @copydoc remote::IDevice::RunSession
        grpc::Status RunSession(grpc::ServerContext* context, const remote::SessionDetailsTo* request, google::protobuf::Empty*) override;
        /// @copydoc remote::IDevice::WaitForSession
        grpc::Status WaitForSession(grpc::ServerContext* context, const google::protobuf::Empty*, ::grpc::ServerWriter<remote::RawKeys>* writer) override;
        /// @copydoc remote::IDevice::GetLinkStatus
        grpc::Status GetLinkStatus(grpc::ServerContext* context, const google::protobuf::Empty*, ::grpc::ServerWriter<remote::LinkStatus>* writer) override;

        ///@}

        ///@{
        /// @name IKeyCallback interface

        /// @copydoc IKeyCallback::OnKeyGeneration
        void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
        ///@}

        grpc::Status RegisterWithSiteAgent(const std::string& address);
    protected: // members
        std::shared_ptr<IQKDDevice> device;
        std::shared_ptr<grpc::ServerCredentials> creds;
        URI sessionAddress;
        using KeyListList = std::vector<std::unique_ptr<KeyList>>;
        KeyListList recievedKeys;
        std::condition_variable recievedKeysCv;
        std::mutex recievedKeysMutex;
        bool shutdown = false;
    protected: // methods
        void ProcessKeys(::grpc::ServerWriter<remote::RawKeys>* writer);
    };

} // namespace cqp


