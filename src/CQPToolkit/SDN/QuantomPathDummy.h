/*!
* @file
* @brief QuantomPathDummy
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/IQuantumPath.grpc.pb.h"
#include <grpc++/security/server_credentials.h>

namespace cqp
{

    /**
     * @brief The QuantomPathDummy class
     * A class for simulating a QPA algorithm
     */
    class CQPTOOLKIT_EXPORT QuantomPathDummy : public remote::IQuantumPath::Service
    {
    public:
        /**
         * @brief QuantomPathDummy
         * Constructor
         */
        QuantomPathDummy() = default;

        ~QuantomPathDummy();

        /**
         * @brief StartServer
         * @param port port number of the server
         * @param creds
         */
        void StartServer(int& port, std::shared_ptr<grpc::ServerCredentials> creds);

        ///@{
        /// @name remote::IQuantumPath interface

        /// @copydoc remote::IQuantumPath::GetPath
        /// @param context Connection details from the server
        grpc::Status GetPath(grpc::ServerContext* context,
                             const remote::PhysicalPathSpec* request, remote::PhysicalPath* response) override;

        /// @copydoc remote::IQuantumPath::CreatePath
        /// @param context Connection details from the server
        grpc::Status CreatePath(grpc::ServerContext* context, const remote::PhysicalPath* request, google::protobuf::Empty*) override;
        ///@}

    protected:
        /// my server
        std::unique_ptr<grpc::Server> server;

    }; // class QuantomPathDummy
} // namespace cqp


