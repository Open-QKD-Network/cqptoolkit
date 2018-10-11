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
#pragma once
#include <condition_variable>
#include <QKDInterfaces/IHello.grpc.pb.h>

namespace grpc
{
    class ChannelCredentials;
    class ServerCredentials;
}

namespace cqp
{
    namespace net
    {

        /**
         * @brief The TwoWayServerConnector class
         * Establishes a reverse client connection for a server so that the pair can
         * call each other
         */
        class TwoWayServerConnector : public remote::IHello::Service
        {
        public:
            /**
             * @brief TwoWayServerConnector
             * Constructor
             * @param creds credentials to use when connecting to peer
             */
            TwoWayServerConnector(std::shared_ptr<grpc::ChannelCredentials> creds);
            /**
             * @brief Connect
             * Connect as a client and wait for the server to connect to us.
             * @param address Server address to contact
             * @return Status of command
             */
            grpc::Status Connect(std::string address);

            /**
             * @brief Disconnect
             */
            void Disconnect();

            ///@{
            /// @name IHello interface

            /**
             * @brief ConnectToMe
             * @param context
             * @param request
             * @param response
             * @return status
             */
            grpc::Status ConnectToMe(grpc::ServerContext *context, const remote::Connection *request, google::protobuf::Empty *response) override;
            ///@}

            /**
             * @brief WaitForClient
             * Blocks until the contacted server has connected as a client
             * @param timeout How long to wait.
             * @return true if the server connected in time
             */
            std::shared_ptr<grpc::Channel> WaitForClient(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

            /**
             * @brief GetClient
             * @return The client connection
             */
            std::shared_ptr<grpc::Channel> GetClient();

            /**
             * @brief SetServerAddress
             * @param newAddress The address to use to contact this server
             */
            void SetServerAddress(const std::string& newAddress);
        protected:
            /// Has the remote server contacted us
            bool connectToMeCalled = false;
            /// The channel to the other side
            std::shared_ptr<grpc::Channel> clientChannel;
            /// our address used to contact us.
            std::string serverAddress;
            /// credentials to use when connecting to peer
            std::shared_ptr<grpc::ChannelCredentials> clientCreds;

            /// notify when a client connects
            std::condition_variable cv;
            /// access protection
            std::mutex cvMutex;
        }; // TwoWayServerConnector
    } // namespace net
} // namespace cqp
