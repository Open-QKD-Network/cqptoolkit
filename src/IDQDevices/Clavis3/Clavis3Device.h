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
#include <string>
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "IDQDevices/idqdevices_export.h"

namespace cqp
{

    class IDQDEVICES_EXPORT Clavis3Device :
        public virtual IQKDDevice,
        public virtual ISessionController,
        public IKeyPublisher
    {
    public:
        Clavis3Device(const std::string& hostname);
        ~Clavis3Device() override;

        ///@{
        /// @name IQKDDevice interface
        std::string GetDriverName() const override;
        URI GetAddress() const override;
        bool Initialise(config::DeviceConfig& parameters) override;
        ISessionController* GetSessionController() override;
        IKeyPublisher* GetKeyPublisher() override;
        remote::Device GetDeviceDetails() override;
        std::vector<stats::StatCollection*> GetStats() override;
        ///@}

        // ISessionController interface
    public:
        grpc::Status StartServer(const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;
        grpc::Status StartServerAndConnect(URI otherController, const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds) override;
        std::string GetConnectionAddress() const override;
        grpc::Status StartSession() override;
        void EndSession() override;
        void WaitForEndOfSession() override;
    protected: // methods
        void PassOnKeys();
    protected: // members
        class Impl;
        std::unique_ptr<Impl> pImpl;

    };

} // namespace cqp


