/*!
* @file
* @brief Server
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Server.h"
#include "CQPToolkit/Util/Logger.h"
#include <sys/types.h>
#include <sys/socket.h>

namespace cqp
{
    namespace net
    {

        Server::Server()
        {

        }

        Server::Server(const SocketAddress& listenAddress)
        {
            Listen(listenAddress);
        }

        void Server::Listen(const SocketAddress& listenAddress)
        {
            if(Bind(listenAddress))
            {
                if(::listen(handle, 1) != 0)
                {
                    LOGERROR("Failed to listen on socket: " + strerror(errno));
                }
            }
            else
            {
                LOGERROR("Failed to bind to " + listenAddress.ToString());
            }
        }

        std::shared_ptr<Stream> Server::AcceptConnection()
        {
            std::shared_ptr<Stream> result;
            struct sockaddr_storage clientAddr;
            socklen_t clientAddrLen = 0;
            int clientHandle = ::accept(handle, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

            if(clientHandle >= 0)
            {
                result.reset(new Stream(clientHandle));
            }

            return result;
        }

    } // namespace net
} // namespace cqp
