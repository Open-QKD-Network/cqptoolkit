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
#include "QuantomPathDummy.h"
#include <grpc++/server_builder.h>
#include <grpc++/server.h>

namespace cqp
{

    QuantomPathDummy::~QuantomPathDummy()
    {
        server->Shutdown();
    }

    void QuantomPathDummy::StartServer(int& port, std::shared_ptr<grpc::ServerCredentials> creds)
    {

        grpc::ServerBuilder builder;

        builder.AddListeningPort("localhost:" + std::to_string(port), creds, &port);
        builder.RegisterService(this);

        server = builder.BuildAndStart();
    }

    grpc::Status QuantomPathDummy::GetPath(grpc::ServerContext*, const remote::PhysicalPathSpec* request, remote::PhysicalPath* response)
    {
        remote::HopPair hopPair;
        (*hopPair.mutable_first()) = request->src();
        (*hopPair.mutable_second()) = request->dest();

        (*response->mutable_hops()->Add()) = hopPair;
        return grpc::Status::OK;
    }

    grpc::Status QuantomPathDummy::CreatePath(grpc::ServerContext*, const cqp::remote::PhysicalPath*, google::protobuf::Empty*)
    {
        return grpc::Status::OK;
    }
}

