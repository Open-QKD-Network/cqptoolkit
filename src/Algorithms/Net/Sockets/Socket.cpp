/*!
* @file
* @brief Socket
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Socket.h"
#include <cstring>
#include "Algorithms/Logging/Logger.h"
#include "Net/DNS.h"
#include <memory>
#include <iomanip>
#include <algorithm>
#if defined (__unix__)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
#elif defined(WIN32)
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
#endif

namespace cqp
{
    namespace net
    {

        bool Socket::Bind(const SocketAddress& address)
        {
            bool result = false;
            unsigned int addrSize {};
            std::unique_ptr<const struct sockaddr> addr {address.ToStruct(addrSize)};

            int bindResult = bind(handle, addr.get(), addrSize);
            if(bindResult == 0)
            {
                result = true;
            }
            else
            {
                LOGERROR(::strerror(errno));
            }

            return result;
        }

        bool Socket::SetReceiveTimeout(std::chrono::milliseconds timeout)
        {
            bool result = false;
            try
            {

                using namespace std::chrono;
                struct timeval tv {};
                tv.tv_sec = duration_cast<seconds>(timeout).count();
                tv.tv_usec = duration_cast<microseconds>(timeout).count() - (tv.tv_sec * 1000 * 1000);
                if(handle != 0)
                {
                    result = setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
                    if(!result)
                    {
                        LOGERROR(::strerror(errno));
                    }
                }
                else
                {
                    LOGERROR("Socket not setup");
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return result;
        }

        void Socket::Close()
        {
            ::close(handle);
        }

        Socket::~Socket()
        {
            Socket::Close();
        }

        SocketAddress Socket::GetAddress() const
        {
            SocketAddress result;
            struct sockaddr_storage addr {};
            socklen_t length = sizeof(addr);
            if(getsockname(handle, reinterpret_cast<sockaddr*>(&addr), &length) == 0)
            {
                result.FromStruct(addr);
            }
            return result;
        }

        bool Socket::SetBlocking(bool active)
        {

#if defined(WIN32)
            unsigned long mode = 0;
            if(!active)
            {
                mode = 1;
            }
            return ioctlsocket(handle, FIONBIO, &mode) == 0;
#elif defined(__unix__)
            int flags = fcntl(handle, F_GETFL, 0);
            if (flags != -1)
            {
                if(active)
                {
                    // zero the non-blocking flag
                    flags &= ~O_NONBLOCK;
                }
                else
                {
                    // set the blocking flag
                    flags |= O_NONBLOCK;
                }
            }
            return fcntl(handle, F_SETFL, flags) == 0;
#endif
        }

        bool Socket::IsBlocking()
        {
            int flags = fcntl(handle, F_GETFL, 0);
            return (flags & O_NONBLOCK) == 0;
        }

        IPAddress::IPAddress() :
            ip6({})
        {
        }

        IPAddress::IPAddress(const std::string& input) : IPAddress()
        {
            using namespace std;
            if(input.find('.') != string::npos)
            {
                ::inet_pton(AF_INET, input.c_str(), reinterpret_cast<int*>(&ip4));
            }
            else if(input.find(':') != string::npos)
            {
                ::inet_pton(AF_INET6, input.c_str(), reinterpret_cast<int*>(&ip6));
            }
        }

        std::string IPAddress::ToString() const
        {
            using namespace std;
            std::stringstream result;

            if(isIPv4)
            {
                const char* dot = ".";
                result << std::to_string(ip4[0]) + dot +
                       std::to_string(ip4[1]) + dot +
                       std::to_string(ip4[2]) + dot +
                       std::to_string(ip4[3]);
            }
            else
            {
                const char* colon = ":";
                result << hex << setw(2) << setfill('0') << static_cast<unsigned>(ip6[0]) << setw(2) << setfill('0') << static_cast<unsigned>(ip6[1]);
                for(size_t index = 2; index < ip6.size(); index += 2)
                {
                    result << colon << hex << setw(2) << setfill('0') << static_cast<unsigned>(ip6[index]) << setw(2) << setfill('0') << static_cast<unsigned>(ip6[index + 1]);
                }
            }
            return result.str();
        }

        void IPAddress::FromStruct(const sockaddr_storage& addr)
        {
            using Addr4Ptr = const struct sockaddr_in*;
            using Addr6Ptr = const struct sockaddr_in6*;

            if(addr.ss_family == AF_INET)
            {
                isIPv4 = true;
                Addr4Ptr addr4 = reinterpret_cast<Addr4Ptr>(&addr);
                const unsigned char* arraycast = reinterpret_cast<const unsigned char*>(&addr4->sin_addr.s_addr);
                for(size_t index = 0; index < 4; index++)
                {
                    ip4[index] = arraycast[index];
                }
            }
            else if(addr.ss_family == AF_INET6)
            {
                isIPv4 = false;
                Addr6Ptr addr6 = reinterpret_cast<Addr6Ptr>(&addr);
                const unsigned char* arraycast = reinterpret_cast<const unsigned char*>(&addr6->sin6_addr.__in6_u.__u6_addr8);
                for(size_t index = 0; index < 16; index++)
                {
                    ip6[index] = arraycast[index];
                }
            }
        }


        void IPAddress::FromStruct(const sockaddr& addr)
        {
            FromStruct(reinterpret_cast<const sockaddr_storage&>(addr));
        }

        std::unique_ptr<const struct sockaddr> IPAddress::ToStruct(unsigned int& size) const
        {
            sockaddr* result = nullptr;

            if(isIPv4)
            {
                struct sockaddr_in* addr4 = new sockaddr_in();
                *addr4 = {};
                addr4->sin_family = AF_INET;
                if(IsNull())
                {
                    addr4->sin_addr.s_addr = INADDR_ANY;
                }
                else
                {
                    unsigned char* addrPtr = reinterpret_cast<unsigned char*>(&addr4->sin_addr.s_addr);
                    std::copy(ip4.begin(), ip4.end(), addrPtr);
                }
                result = reinterpret_cast<struct sockaddr*>(addr4);
                size = sizeof(*addr4);
            }
            else
            {
                struct sockaddr_in6* addr6 = new sockaddr_in6();
                *addr6 = {};
                addr6->sin6_family = AF_INET6;
                std::copy(ip6.begin(), ip6.end(), addr6->sin6_addr.__in6_u.__u6_addr8);
                result = reinterpret_cast<struct sockaddr*>(addr6);
                size = sizeof(*addr6);
            }
            return std::unique_ptr<struct sockaddr>(result);
        }

        bool IPAddress::IsNull() const
        {
            bool result = true;
            if(isIPv4)
            {
                result = std::all_of(ip4.begin(), ip4.end(), [](const auto& item) { return item == 0; });
            }
            else
            {
                result = std::all_of(ip6.begin(), ip6.end(), [](const auto& item) { return item == 0; });
            }
            return result;
        }

        SocketAddress::SocketAddress(const std::string& value)
        {
            using namespace std;
            size_t colonPos = value.find(':');
            net::ResolveAddress(value.substr(0, colonPos), ip);
            if(colonPos != string::npos)
            {
                try
                {
                    port = static_cast<uint16_t>(stoul(value.substr(colonPos + 1)));
                }
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                }
            }
        }

        std::string SocketAddress::ToString() const
        {
            std::string result {ip.ToString()};
            if(port != 0)
            {
                result += ":" + std::to_string(port);
            }
            return result;
        }

        std::unique_ptr<const struct sockaddr> SocketAddress::ToStruct(unsigned int& size) const
        {
            sockaddr* result = nullptr;

            if(ip.isIPv4)
            {
                struct sockaddr_in* addr4 = new sockaddr_in();
                *addr4 = {};
                addr4->sin_family = AF_INET;
                addr4->sin_port = htons(port);
                if(ip.IsNull())
                {
                    addr4->sin_addr.s_addr = INADDR_ANY;
                }
                else
                {
                    unsigned char* addrPtr = reinterpret_cast<unsigned char*>(&addr4->sin_addr.s_addr);
                    std::copy(ip.ip4.begin(), ip.ip4.end(), addrPtr);
                }
                result = reinterpret_cast<struct sockaddr*>(addr4);
                size = sizeof(*addr4);
            }
            else
            {
                struct sockaddr_in6* addr6 = new sockaddr_in6();
                *addr6 = {};
                addr6->sin6_family = AF_INET6;
                addr6->sin6_port = htons(port);
                std::copy(ip.ip6.begin(), ip.ip6.end(), addr6->sin6_addr.__in6_u.__u6_addr8);
                result = reinterpret_cast<struct sockaddr*>(addr6);
                size = sizeof(*addr6);
            }
            return std::unique_ptr<struct sockaddr>(result);
        }

        void SocketAddress::FromStruct(const sockaddr_storage& addr)
        {
            using Addr4Ptr = const struct sockaddr_in*;
            using Addr6Ptr = const struct sockaddr_in6*;

            ip.FromStruct(addr);

            if(addr.ss_family == AF_INET)
            {
                Addr4Ptr addr4 = reinterpret_cast<Addr4Ptr>(&addr);
                port = ntohs(addr4->sin_port);
            }
            else if(addr.ss_family == AF_INET6)
            {
                Addr6Ptr addr6 = reinterpret_cast<Addr6Ptr>(&addr);
                port = ntohs(addr6->sin6_port);
            }
        }

        bool Socket::Read(void* data, size_t length, size_t& bytesReceived)
        {
            bool result = false;
            ssize_t received = ::read(handle, data, length);
            if(received >= 0)
            {
                bytesReceived = static_cast<size_t>(received);
                result = true;
            }
            else if(errno == EAGAIN || errno == EINTR)
            {
                // timeout
                bytesReceived = 0;
                result = true;
            }
            else
            {
                LOGERROR("Socket Error:" + std::to_string(errno) + " " +  ::strerror(errno));
            }
            return result;
        }

        bool Socket::Write(const void* data, size_t length)
        {
            bool result = true;
            size_t sent = 0;
            ssize_t sentThisTime = 0;

            while(sentThisTime >= 0 && sent < length)
            {
                sentThisTime += ::write(handle, data, length);
                if(sentThisTime > 0)
                {
                    sent += static_cast<size_t>(sentThisTime);
                }
                else
                {
                    LOGERROR(::strerror(errno));
                    result = false;
                }
            }

            return result;
        }

    } // namespace net
} // namespace cqp
