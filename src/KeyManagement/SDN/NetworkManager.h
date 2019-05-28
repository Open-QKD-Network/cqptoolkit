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
#include <grpcpp/security/credentials.h>

namespace cqp
{

    /**
     * @brief The NetworkManager class provides basic static network configuration
     */
    class KEYMANAGEMENT_EXPORT NetworkManager :
        public remote::INetworkManager::Service
    {
    public:
        /**
         * @brief NetworkManager
         * @param staticPaths
         * @param creds
         */
        NetworkManager(google::protobuf::RepeatedPtrField<remote::PhysicalPath> staticPaths,
                       std::shared_ptr<grpc::ChannelCredentials> creds);

        /// distructor
        ~NetworkManager() override;
        ///@{
        /// @name INetworkManager interface

        /// @copydoc cqp::remote::INetworkManager::RegisterSite
        grpc::Status RegisterSite(grpc::ServerContext*, const remote::Site* request, google::protobuf::Empty*) override;
        /// @copydoc cqp::remote::INetworkManager::UnregisterSite
        grpc::Status UnregisterSite(grpc::ServerContext*, const remote::SiteAddress* request, google::protobuf::Empty*) override;
        /// @copydoc cqp::remote::INetworkManager::GetRegisteredSites
        grpc::Status GetRegisteredSites(grpc::ServerContext*, const google::protobuf::Empty*, remote::SiteDetailsList* response) override;

        ///@}

        /**
         * @brief StopActiveLinks
         */
        void StopActiveLinks();

    protected:
        /// known sites
        std::map<std::string, remote::Site> sites;
        /// access control for sites
        std::mutex sitesMutex;

        /// a list of whether deivces are ready to use
        using DeviceReadyList = std::map<std::string /*device id*/, bool>;
        /// a list of devices by site
        using SiteDeviceList = std::map<std::string /*site*/, DeviceReadyList>;

        /**
         * @brief The Link struct
         */
        struct Link
        {
            /**
             * @brief Link
             * @param path
             */
            Link(const remote::PhysicalPath& path);

            /// the path details
            const remote::PhysicalPath path;
            /// the sites associated with this path
            SiteDeviceList sites;
            /// has this link been activated
            bool active {false};
        };

        /// known links
        std::vector<Link> links;

        /// credentials for talking with clients
        std::shared_ptr<grpc::ChannelCredentials> creds;

    protected: // methods

        /**
         * @brief CheckLink
         * See if this link can be activated
         * @param link
         */
        void CheckLink(Link& link);

        /**
         * @brief StartLink
         * Start a link
         * @param link
         */
        void StartLink(Link& link);

        /**
         * @brief StopLink
         * Stop a link
         * @param link
         */
        void StopLink(Link& link);
    };

} // namespace cqp


