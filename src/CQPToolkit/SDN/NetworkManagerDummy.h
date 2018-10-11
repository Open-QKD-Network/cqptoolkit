/*!
* @file
* @brief NetworkManagerDummy
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/INetworkManager.grpc.pb.h"
#include <grpc++/security/server_credentials.h>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp
{
    /**
     * @brief The NetworkManagerDummy class
     */
    class CQPTOOLKIT_EXPORT NetworkManagerDummy : public remote::INetworkManager::Service
    {
    public:
        /**
         * @brief NetworkManagerDummy
         * Constructor
         */
        NetworkManagerDummy();

        virtual ~NetworkManagerDummy() override;

        ///@{
        /// @name remote::INetworkManager

        /// @copydoc remote::INetworkManager::RegisterSite
        /// @param context Connection details from the server
        grpc::Status RegisterSite(grpc::ServerContext* context,
                                  const remote::Site* request, google::protobuf::Empty*) override;

        /// @copydoc remote::INetworkManager::UnregisterSite
        /// @param context Connection details from the server
        grpc::Status UnregisterSite(grpc::ServerContext* context,
                                    const remote::SiteAddress* request, google::protobuf::Empty*) override;

        /// @}

        /**
         * @brief StartServer
         * @param port port number of the server
         * @param creds
         */
        void StartServer(int& port, std::shared_ptr<grpc::ServerCredentials> creds);

    protected:
        /// my server
        std::unique_ptr<grpc::Server> server;
    }; // class NetworkManagerDummy
} // namespace cqp
