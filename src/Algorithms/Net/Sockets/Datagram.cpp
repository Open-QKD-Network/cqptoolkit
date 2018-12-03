/*!
* @file
* @brief Datagram
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Datagram.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Logging/Logger.h"
#if defined (__unix__)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netdb.h>
#elif defined(WIN32)
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
#endif

namespace cqp
{
    namespace net
    {

        Datagram::Datagram()
        {
            handle = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(handle == 0)
            {
                LOGERROR("Failed to create socket: " + strerror(errno));
            }
        }

        Datagram::Datagram(const std::string& bindAddress, uint16_t sourcePort)
        {
            SocketAddress address(bindAddress);
            address.port = sourcePort;

            if(address.ip.isIPv4)
            {
                handle = ::socket(AF_INET, SOCK_DGRAM, 0);
            }
            else
            {
                handle = ::socket(AF_INET6, SOCK_DGRAM, 0);
            }

            if(handle == 0)
            {
                LOGERROR("Failed to create socket: " + strerror(errno));
            }

            Bind(address);
        }

        bool Datagram::SendTo(const void* data, size_t length, const SocketAddress& destination)
        {
            unsigned int addrSize {};
            std::unique_ptr<const struct ::sockaddr> addr{destination.ToStruct(addrSize)};
            const void* sendFrom = data;
            size_t bytesSent = 0;
            while(bytesSent < length)
            {
                ssize_t sentThisTime = sendto(handle, sendFrom, length - bytesSent, 0, addr.get(), addrSize);
                if(sentThisTime > 0)
                {
                    bytesSent += static_cast<size_t>(sentThisTime);
                    sendFrom = reinterpret_cast<const unsigned char*>(sendFrom) + sentThisTime;
                }
                else
                {
                    LOGERROR(strerror(errno));
                    break; // while
                }
            }

            return bytesSent == length;
        }

        bool Datagram::ReceiveFrom(void* data, size_t length, size_t& bytesReceived, SocketAddress& sender)
        {
            bool result =false;
            struct sockaddr_storage addr {};
            unsigned int addrSize = sizeof(addr);
            ssize_t received = recvfrom(handle, data, length, 0, reinterpret_cast<struct sockaddr*>(&addr), &addrSize);
            if(received > 0)
            {
                bytesReceived = static_cast<size_t>(received);

                sender.FromStruct(addr);
                result = true;
            }
            else
            {
                LOGERROR(strerror(errno));
            }
            return result;
        }

    } // namespace net
} // namespace cqp
