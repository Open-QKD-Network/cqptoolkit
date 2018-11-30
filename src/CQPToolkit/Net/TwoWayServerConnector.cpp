/*!
* @file
* @brief TwoWayServerConnector
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TwoWayServerConnector.h"
#include <grpc++/security/credentials.h>
#include <grpc++/server_builder.h>
#include <grpc++/create_channel.h>
#include "CQPAlgorithms/Logging/Logger.h"
#include <thread>
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{
    namespace net
    {

        grpc::Status cqp::net::TwoWayServerConnector::Connect(std::string address)
        {
            using std::chrono::system_clock;
            using std::chrono::seconds;
            using namespace grpc;
            Status result = Status::OK;
            // if the other side hasn't connected to us yet...
            if(clientChannel == nullptr)
            {
                {
                    using namespace std;
                    lock_guard<mutex> lock(cvMutex);
                    // create a channel to the other side
                    clientChannel = CreateChannel(address, clientCreds);
                }

                LOGINFO("Waiting for connection from " + address + "...");
                if(!clientChannel->WaitForConnected(system_clock::now() + seconds(10)))
                {
                    result = LogStatus(Status(StatusCode::INVALID_ARGUMENT, "Failed to connect"));
                    clientChannel = nullptr;
                }
                // prevent recursive calling
                else if(!connectToMeCalled)
                {
                    LOGINFO("Connected.");
                    if(address.empty())
                    {
                        LOGWARN("Invalid address for local server, reverse connection may fail.");
                    }
                    remote::Connection myConn;
                    google::protobuf::Empty response;
                    (*myConn.mutable_address()) = serverAddress;
                    LOGDEBUG("Requesting reverse connection.");
                    grpc::ClientContext ctx;
                    // ... ask the other side to call us back
                    result = LogStatus(
                                 remote::IHello::NewStub(clientChannel)->ConnectToMe(&ctx, myConn, &response));
                } // else if(!connectToMeCalled)
                else
                {
                    LOGINFO("Connected.");
                }
            } // if(clientChannel == nullptr)
            else
            {
                LOGINFO("Already connected");
            } // else
            return result;
        } // Connect

        TwoWayServerConnector::TwoWayServerConnector(std::shared_ptr<grpc::ChannelCredentials> creds) :
            clientCreds{creds}
        {
        }

        void TwoWayServerConnector::Disconnect()
        {
            using namespace std;
            lock_guard<mutex> lock(cvMutex);
            clientChannel = nullptr;
            connectToMeCalled = false;
        } // Disconnect

        grpc::Status TwoWayServerConnector::ConnectToMe(grpc::ServerContext*, const cqp::remote::Connection* request, google::protobuf::Empty*)
        {
            connectToMeCalled = true;
            // complete the connection in both directions
            grpc::Status result = LogStatus(
                                      Connect(request->address()));
            // trigger any waiting threads
            cv.notify_all();
            return result;
        } // ConnectToMe

        std::shared_ptr<grpc::Channel> cqp::net::TwoWayServerConnector::WaitForClient(std::chrono::milliseconds timeout)
        {
            using namespace std;
            unique_lock<mutex> lock(cvMutex);
            cv.wait_for(lock, timeout, [&]()
            {
                return clientChannel != nullptr;
            });
            return clientChannel;
        } // WaitForClient

        std::shared_ptr<grpc::Channel> cqp::net::TwoWayServerConnector::GetClient()
        {
            using namespace std;
            lock_guard<mutex> lock(cvMutex);
            return clientChannel;
        } // GetClient

        void cqp::net::TwoWayServerConnector::SetServerAddress(const std::string& newAddress)
        {
            serverAddress = newAddress;
        } // SetServerAddress

    } // namespace net
} // namespace cqp
