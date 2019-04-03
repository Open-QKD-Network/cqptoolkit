/*!
* @file
* @brief NetworkManager
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/INetworkManager.grpc.pb.h"
#include "KeyManagement/keymanagement_export.h"
#include <condition_variable>
#include <thread>

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{

    class KEYMANAGEMENT_EXPORT NetworkManager :
        public remote::INetworkManager::Service
    {
    public:
        NetworkManager(google::protobuf::RepeatedPtrField<remote::PhysicalPath> staticPaths,
                       std::shared_ptr<grpc::ChannelCredentials> creds);

        ~NetworkManager() override;
        ///@{
        /// @name INetworkManager interface

        /// @copydoc INetworkManager::RegisterSite
        grpc::Status RegisterSite(grpc::ServerContext*, const remote::Site* request, google::protobuf::Empty*) override;
        /// @copydoc INetworkManager::UnregisterSite
        grpc::Status UnregisterSite(grpc::ServerContext*, const remote::SiteAddress* request, google::protobuf::Empty*) override;
        /// @copydoc INetworkManager::GetRegisteredSites
        grpc::Status GetRegisteredSites(grpc::ServerContext*, const google::protobuf::Empty*, remote::SiteDetailsList* response) override;

        ///@}

        void StopActiveLinks();

    protected:
        std::map<std::string, remote::Site> sites;
        std::mutex sitesMutex;

        using DeviceReadyList = std::map<std::string /*device id*/, bool>;
        using SiteDeviceList = std::map<std::string /*site*/, DeviceReadyList>;

        struct Link
        {
            Link(const remote::PhysicalPath& path);

            const remote::PhysicalPath path;
            SiteDeviceList sites;
            bool active {false};
        };

        std::vector<Link> links;

        std::shared_ptr<grpc::ChannelCredentials> creds;

    protected: // methods

        void CheckLink(Link& link);

        void StartLink(Link& link);
        void StopLink(Link& link);
    };

} // namespace cqp


