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

namespace cqp
{

    class IQKDDevice;
    namespace remote
    {
        class DeviceConfig;
    }

    /**
     * @brief The RemoteQKDDevice class simplifies the interface to the device down to start/stop and Key retreval
     * @details
        skinparam package {
            backgroundColor<<CQPToolkit>> DarkKhaki
        }

        package "HandeldDriver (Alice)" <<CQPToolkit>> {
            component RemoteQKDDriver as aliceDrv
            component LEDDriver as aliceDev
            aliceDrv .up.> aliceDev
        }
        interface "IDevice" as aliceDevice
        aliceDrv -down- aliceDevice

        package "FreespaceBobDriver (Bob)" <<CQPToolkit>> {
            component RemoteQKDDriver as bobDrv
            component Mk1PhotonDetector as bobDev
            bobDrv .up.> bobDev
        }
        interface "IDevice" as bobDevice
        bobDrv -down- bobDevice

        package ClientApp1 {
            component KeyReader as kr1
        }
        kr1 .up.>aliceDevice

        package ClientApp2 {
            component KeyReader as kr2
        }
        kr2 .up.> bobDevice
     */
    class CQPTOOLKIT_EXPORT RemoteQKDDevice :
        public virtual IKeyCallback,
        public remote::IDevice::Service
    {
    public:
        RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                        std::shared_ptr<grpc::ServerCredentials> creds = grpc::InsecureServerCredentials());

        ~RemoteQKDDevice() override;

        ///@{
        /// @name remote::IDevice interface

        /// @copydoc remote::IDevice::RunSession
        grpc::Status RunSession(grpc::ServerContext* context, const remote::SessionDetailsTo* request, google::protobuf::Empty*) override;
        /// @copydoc remote::IDevice::WaitForSession
        grpc::Status WaitForSession(grpc::ServerContext* context, const remote::LocalSettings*settings, ::grpc::ServerWriter<remote::RawKeys>* writer) override;
        /// @copydoc remote::IDevice::GetLinkStatus
        grpc::Status GetLinkStatus(grpc::ServerContext* context, const google::protobuf::Empty*, ::grpc::ServerWriter<remote::LinkStatus>* writer) override;
        /// @copydoc remote::IDevice::EndSession
        grpc::Status EndSession(grpc::ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*) override;

        /// @copydoc remote::IDevice::GetDetails
        grpc::Status GetDetails(grpc::ServerContext*, const google::protobuf::Empty*, remote::ControlDetails*response) override;

        ///@}

        ///@{
        /// @name IKeyCallback interface

        /// @copydoc IKeyCallback::OnKeyGeneration
        void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
        ///@}

        grpc::Status RegisterWithSiteAgent(const std::string& address);

        void UnregisterWithSiteAgent();

        bool StartControlServer(const std::string& controlAddress, const std::string& siteAgent = "");

        void WaitForServerShutdown();

        void StopServer();

        std::string GetControlAddress()
        {
            return qkdDeviceAddress;
        }
    protected: // members
        std::shared_ptr<IQKDDevice> device;
        std::shared_ptr<grpc::ServerCredentials> creds;
        using KeyListList = std::vector<std::unique_ptr<KeyList>>;
        KeyListList recievedKeys;
        std::condition_variable recievedKeysCv;
        std::mutex recievedKeysMutex;
        std::atomic_bool shutdown {false};
        std::string qkdDeviceAddress;
        std::string siteAgentAddress;
        std::unique_ptr<grpc::Server> deviceServer;
    protected: // methods
        grpc::Status ProcessKeys(grpc::ServerContext* ctx, ::grpc::ServerWriter<remote::RawKeys>* writer);

    };

} // namespace cqp


