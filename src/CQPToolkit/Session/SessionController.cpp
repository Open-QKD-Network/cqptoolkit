/*!
* @file
* @brief SessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SessionController.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include "CQPToolkit/Session/PublicKeyService.h"
#include "CQPToolkit/Net/TwoWayServerConnector.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPAlgorithms/Random/RandomNumber.h"
#include "CQPToolkit/Net/DNS.h"

namespace cqp
{

    using google::protobuf::Empty;
    using grpc::Status;
    using grpc::StatusCode;
    using grpc::ClientContext;

    SessionController::SessionController(std::shared_ptr<grpc::ChannelCredentials> creds)
    {
        pubKeyServ = new PublicKeyService();
        twoWayComm = new net::TwoWayServerConnector(creds);
        rng = new RandomNumber();

    } // SessionController

    grpc::Status SessionController::StartSession(const remote::OpticalParameters&)
    {
        Status result;
        // make sure the other side has connected to us

        auto channel = twoWayComm->WaitForClient();
        std::unique_ptr<remote::ISession::Stub> otherController;
        if(channel)
        {
            otherController = remote::ISession::NewStub(channel);
        }
        if(otherController)
        {
            ClientContext ctx;
            remote::SessionDetails request;
            Empty response;

            request.set_peeraddress(myAddress);
            request.set_keytoken(keyToken);

            // send the command to the other side
            result = otherController->SessionStarting(&ctx, request, &response);

        } // if(otherController)
        else
        {
            result = Status(StatusCode::FAILED_PRECONDITION, "invalid remote session controller");
        } // else
        return result;
    } // StartSession

    void SessionController::EndSession()
    {
        Empty request;
        Empty response;
        ClientContext ctx;
        auto channel = twoWayComm->GetClient();
        std::unique_ptr<remote::ISession::Stub> otherController;
        if(channel)
        {
            otherController = remote::ISession::NewStub(channel);
        }

        if(otherController)
        {
            // send the command to the other side
            LogStatus(
                otherController->SessionEnding(&ctx, request, &response));
        } // if otherController

        twoWayComm->Disconnect();
        pairedControllerUri.clear();
    } // EndSession

    SessionController::~SessionController()
    {
        SessionController::EndSession();
        if(server)
        {
            server->Shutdown();
            server = nullptr;
        } // if server
    } // ~SessionController

    Status SessionController::SessionStarting(grpc::ServerContext*, const remote::SessionDetails*, Empty*)
    {
        // nothing to do
        return Status();
    } // SessionStarting

    Status SessionController::SessionEnding(grpc::ServerContext*, const Empty*, Empty*)
    {
        twoWayComm->Disconnect();
        pairedControllerUri.clear();
        return Status();
    } // SessionEnding

    Status SessionController::StartServer(const std::string& hostname, uint16_t requestedPort, std::shared_ptr<grpc::ServerCredentials> creds)
    {
        Status result;
        if(server == nullptr)
        {
            if(hostname.empty())
            {
                // FIXME: this may not work in all situations
                // The call may just have to specify the hostname manually
                myAddress = net::GetHostname(); // "localhost";
            } // if(hostname.empty())
            else
            {
                myAddress = hostname;
            } // else
            // create our own server which all the steps will use
            grpc::ServerBuilder builder;
            // grpc will create worker threads as it needs, idle work threads
            // will be stopped if there are more than this number running
            // setting this too low causes large number of thread creation+deletions, default = 2
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 50);
            builder.AddListeningPort(myAddress + ":" + std::to_string(requestedPort), creds, &listenPort);

            // register all the classes which provide a remote interface
            builder.RegisterService(this);
            builder.RegisterService(pubKeyServ);
            builder.RegisterService(twoWayComm);
            // allow other steps in the process to register with our service
            RegisterServices(builder);

            // star the server
            server = builder.BuildAndStart();
            if(server)
            {
                LOGINFO("Listening on " + myAddress + ":" + std::to_string(listenPort));
                twoWayComm->SetServerAddress(myAddress + ":" + std::to_string(listenPort));
            } // if(server)
            else
            {
                result = Status(StatusCode::INVALID_ARGUMENT, "Failed to create server");
            } // else
        }
        return result;
    } // StartServer

    Status SessionController::StartServerAndConnect(cqp::URI otherControllerURI, const std::string& hostname, uint16_t listenPort, std::shared_ptr<grpc::ServerCredentials> creds)
    {
        Status result = StartServer(hostname, listenPort, creds);
        if(result.ok())
        {
            using namespace std;
            pairedControllerUri = otherControllerURI;

            // get a two way connection going
            Status result = LogStatus(
                                twoWayComm->Connect(otherControllerURI));
            if(result.ok())
            {
                auto channel = twoWayComm->WaitForClient();
                if(channel)
                {
                    std::unique_ptr<remote::ISession::Stub> otherController = remote::ISession::NewStub(channel);

                    // exchange public keys
                    otherController = remote::ISession::NewStub(twoWayComm->GetClient());
                    result = pubKeyServ->SharePublicKey(twoWayComm->GetClient(), keyToken);
                } //if(twoWayComm->WaitForClient())
                else
                {
                    result = LogStatus(Status(StatusCode::INTERNAL, "Client connection failed"));
                } // else
            } // if(result.ok())

        } // if(result.ok())
        return result;
    } // StartServerAndConnect
} // namespace cqp
