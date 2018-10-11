/*!
* @file
* @brief DNS
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DNS.h"
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Net/Socket.h"
#if defined(__unix__)
    #include <unistd.h>
    #include <limits.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <ifaddrs.h>
#elif defined(WIN32)
    #include <Winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
#endif

namespace cqp
{
    namespace net
    {

        std::string GetHostname(bool fqdn) noexcept
        {
            std::string result;
            try
            {
                char hostname[256] = {};
                if(gethostname(hostname, sizeof(hostname)) < 0)
                {
                    LOGERROR("Failed to get hostname errno =" + std::to_string(errno));
                }

                result = hostname;

                if(fqdn)
                {
                    struct addrinfo* addr {};
                    struct addrinfo hints {};
                    hints.ai_flags = AI_CANONNAME;
                    // request the hostname be resolved
                    int getResult = getaddrinfo(hostname, nullptr, &hints, &addr);
                    if(getResult == 0 && addr && addr->ai_canonname)
                    {
                        result = addr->ai_canonname;
                    }
                }
            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }

            return result;
        }

        std::vector<IPAddress> GetHostIPs() noexcept
        {
            std::vector<IPAddress> result;
            try
            {

                struct ifaddrs * ifAddrStruct = nullptr;
                if(getifaddrs(&ifAddrStruct) < 0)
                {
                    LOGERROR("Failed to get ip addresses, errno =" + std::to_string(errno));
                }
                else
                {
                    for (auto ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
                    {
                        if(ifa->ifa_addr && (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6))
                        {
                            IPAddress ip;
                            ip.FromStruct(reinterpret_cast<struct sockaddr_storage&>(*ifa->ifa_addr));
                            result.push_back(ip);
                        }
                    }
                }

                if (ifAddrStruct!=nullptr)
                {
                    freeifaddrs(ifAddrStruct);
                }

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return result;

        }

        bool ResolveAddress(const std::string& hostname, IPAddress& ip, bool preferIPv6) noexcept
        {
            bool result = false;
            try
            {
                struct addrinfo* addr {};
                // request the hostname be resolved
                int getResult = getaddrinfo(hostname.c_str(), nullptr, nullptr, &addr);

                if(getResult == 0)
                {
                    // the result is a linked list of addresses
                    struct addrinfo* currentAddr = addr;
                    while(currentAddr != nullptr)
                    {
                        struct sockaddr_storage* ipAddress = reinterpret_cast<struct sockaddr_storage*>(currentAddr->ai_addr);
                        // use the ipv6 address if thats all there is or we prefer it
                        if(addr->ai_family == AF_INET6 && (!result || preferIPv6))
                        {
                            // copy the data somewhere usful
                            ip.FromStruct(*ipAddress);

                            result = true;
                            if(preferIPv6)
                            {
                                // we don't want a v4 address, stop processing the results
                                break; //while
                            }
                        }
                        else if(currentAddr->ai_family == AF_INET)
                        {
                            // copy the data somewhere usful
                            ip.FromStruct(*ipAddress);

                            result = true;
                            if(!preferIPv6)
                            {
                                // we dont want a v6 address, stop processing
                                break; // while
                            }
                        }
                        // move to the the next result
                        currentAddr = currentAddr->ai_next;
                    }
                }
                else
                {
                    LOGERROR(gai_strerror(getResult) + ": " + hostname);
                }

                // cleanup
                freeaddrinfo(addr);
            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return result;
        }


    } // namespace net
} // namespace cqp
