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
#include <grpcpp/server.h>

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
     * @startuml
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
     @enduml
     */
    class CQPTOOLKIT_EXPORT RemoteQKDDevice :
        public virtual IKeyCallback,
        public remote::IDevice::Service
    {
    public:
        /**
         * @brief RemoteQKDDevice constructor
         * @param device the device to manage
         * @param creds credentials for connections
         */
        RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                        std::shared_ptr<grpc::ServerCredentials> creds = grpc::InsecureServerCredentials());

        ~RemoteQKDDevice() override;

        ///@{
        /// @name remote::IDevice interface

        /// @copydoc cqp::remote::IDevice::RunSession
        /// @param context
        grpc::Status RunSession(grpc::ServerContext* context, const remote::SessionDetailsTo* request, google::protobuf::Empty*) override;
        /// @copydoc cqp::remote::IDevice::WaitForSession
        /// @param context
        grpc::Status WaitForSession(grpc::ServerContext* context, const remote::LocalSettings* request, ::grpc::ServerWriter<remote::RawKeys>* writer) override;
        /// @copydoc cqp::remote::IDevice::GetLinkStatus
        /// @param context
        grpc::Status GetLinkStatus(grpc::ServerContext* context, const google::protobuf::Empty*, ::grpc::ServerWriter<remote::LinkStatus>* writer) override;
        /// @copydoc cqp::remote::IDevice::EndSession
        grpc::Status EndSession(grpc::ServerContext*, const google::protobuf::Empty*, google::protobuf::Empty*) override;

        /// @copydoc cqp::remote::IDevice::GetDetails
        grpc::Status GetDetails(grpc::ServerContext*, const google::protobuf::Empty*, remote::ControlDetails*response) override;

        ///@}

        ///@{
        /// @name IKeyCallback interface

        /// @copydoc IKeyCallback::OnKeyGeneration
        void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
        ///@}

        /**
         * @brief RegisterWithSiteAgent
         * @param address
         * @return status
         */
        grpc::Status RegisterWithSiteAgent(const std::string& address);

        /**
         * @brief UnregisterWithSiteAgent
         */
        void UnregisterWithSiteAgent();

        /**
         * @brief StartControlServer
         * @param controlAddress
         * @param siteAgent
         * @return true on success
         */
        bool StartControlServer(const std::string& controlAddress, const std::string& siteAgent = "");

        /**
         * @brief WaitForServerShutdown
         */
        void WaitForServerShutdown();

        /**
         * @brief StopServer
         */
        void StopServer();

        /**
         * @brief GetControlAddress
         * @return the address for the control interface
         */
        std::string GetControlAddress()
        {
            return qkdDeviceAddress;
        }
    protected: // members
        /// the device to manage
        std::shared_ptr<IQKDDevice> device;
        /// connection credentials
        std::shared_ptr<grpc::ServerCredentials> creds;
        /// a list of key lists
        using KeyListList = std::vector<std::unique_ptr<KeyList>>;
        /// keys recieved from the device, waiting to be forwarded
        KeyListList recievedKeys;
        /// for waiting on recievedKeys
        std::condition_variable recievedKeysCv;
        /// access control for recievedKeys
        std::mutex recievedKeysMutex;
        /// should the system be shut down
        std::atomic_bool shutdown {false};
        /// The address of the connected device
        std::string qkdDeviceAddress;
        /// the registered site agent
        std::string siteAgentAddress;
        /// The server providing the control services
        std::unique_ptr<grpc::Server> deviceServer;
    protected: // methods
        /**
         * @brief ProcessKeys
         * take keys from recievedKeysMutex and push them to the caller of WaitForSession
         * @param ctx
         * @param writer
         * @return status
         */
        grpc::Status ProcessKeys(grpc::ServerContext* ctx, ::grpc::ServerWriter<remote::RawKeys>* writer);

    };

} // namespace cqp


