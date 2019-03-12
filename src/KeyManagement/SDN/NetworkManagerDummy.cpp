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
#include "NetworkManagerDummy.h"
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include "Algorithms/Net/DNS.h"

namespace cqp
{
    using google::protobuf::Empty;

    NetworkManagerDummy::~NetworkManagerDummy()
    {
        server->Shutdown();
    }

    grpc::Status NetworkManagerDummy::RegisterSite(grpc::ServerContext*, const cqp::remote::Site*, Empty* )
    {

        return grpc::Status::OK;
    }

    grpc::Status NetworkManagerDummy::UnregisterSite(grpc::ServerContext*, const cqp::remote::SiteAddress*, Empty*)
    {

        return grpc::Status::OK;
    }

    void NetworkManagerDummy::StartServer(int& port, std::shared_ptr<grpc::ServerCredentials> creds)
    {

        grpc::ServerBuilder builder;

        builder.AddListeningPort(std::string(net::AnyAddress) + ":" + std::to_string(port), creds, &port);
        builder.RegisterService(this);

        server = builder.BuildAndStart();
    }

}
