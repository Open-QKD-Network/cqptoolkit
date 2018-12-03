/*!
* @file
* @brief Stream
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Stream.h"
#include "Algorithms/Logging/Logger.h"
#if defined (__unix__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <unistd.h>
#elif defined(WIN32)
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
#endif



namespace cqp
{
    namespace net
    {

        Stream::Stream()
        {
            handle = ::socket(AF_INET, SOCK_STREAM, 0);
            if(handle == 0)
            {
                LOGERROR("Failed to create socket: " + strerror(errno));
            }
            else
            {
                int reuseAddr = 1;
                if(::setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) != 0)
                {
                    LOGERROR("Failed to set SO_REUSEADDR");
                }
            }
        }

        bool Stream::Connect(const SocketAddress& address, std::chrono::milliseconds timeout)
        {
            unsigned int addrSize {};
            auto addr = address.ToStruct(addrSize);
            bool changeToNonBlocking = IsBlocking();

            if(changeToNonBlocking)
            {
                SetBlocking(false);
            }

            int connectResult = ::connect(handle, addr.get(), addrSize);
            if(connectResult != 0)
            {
                if(errno != EINPROGRESS)
                {
                    LOGERROR(::strerror(errno));
                }
            }

            fd_set set;
            FD_ZERO(&set);
            FD_SET(handle, &set);

            struct timeval cTimeout {};
            std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(timeout);

            cTimeout.tv_sec  = sec.count();
            cTimeout.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(timeout - sec).count();

            // on timeout, 0 is returned
            connectResult = ::select(handle + 1, nullptr, &set, nullptr, &cTimeout);

            if(connectResult < 0)
            {
                LOGERROR(::strerror(errno));
            }

            if(changeToNonBlocking)
            {
                SetBlocking(true);
            }
            return connectResult > 0;
        }

        bool Stream::SetKeepAlive(bool active)
        {
            int keepAlive = active;
            return ::setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)) == 0;
        }

        Stream::Stream(int fd)
        {
            handle = fd;
        }

    } // namespace net
} // namespace cqp
