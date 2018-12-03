/*!
* @file
* @brief Interface
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Interface.h"
#include "Algorithms/Logging/Logger.h"
#if defined(__linux)
    #include <sys/types.h>
    #include <ifaddrs.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

namespace cqp
{
    namespace net
    {

        /**
         * @brief IsPrivateNet
         * @param addr
         * @return true if the ip address one specified by rfc 1918 for private Internets
         */
        bool IsPrivateNet(const struct sockaddr_in* addr)
        {
            bool result = false;
            if(addr)
            {
                const unsigned char *ip = reinterpret_cast<const unsigned char *>(&addr->sin_addr.s_addr);
                result = ip[0] == 127 || ip[0] == 10 ||
                         (ip[0] == 172 && (ip[1] >= 16 && ip[1] <= 31)) ||
                         (ip[0] == 192 && ip[1] == 168);
            }

            return result;
        }

        std::vector<std::string> Interface::GetInterfaceNames(Interface::InterfaceType interfaceType)
        {
            using namespace std;
            vector<string> items;

#if defined (__linux)
            struct ifaddrs* ifaddr = nullptr;
            if(getifaddrs(&ifaddr) != -1)
            {
                for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
                {
                    if(ifa && ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
                    {
                        if(interfaceType == InterfaceType::Any || IsPrivateNet(reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr)))
                        {
                            items.push_back(ifa->ifa_name);
                        }
                    }
                }
            }
#else
            LOGERROR("Unimplemented");
#endif

            return items;
        }


        std::vector<std::string> Interface::GetInterfaceBroadcast(Interface::InterfaceType interfaceType)
        {
            using namespace std;
            vector<string> items;

#if defined (__linux)
            struct ifaddrs* ifaddr = nullptr;
            if(getifaddrs(&ifaddr) != -1)
            {
                for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
                {
                    if(ifa && ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
                    {
                        const sockaddr_in* ip4addr = reinterpret_cast<const sockaddr_in*>(ifa->ifa_ifu.ifu_broadaddr);

                        if(interfaceType == InterfaceType::Any || IsPrivateNet(ip4addr))
                        {
                            if(std::string(ifa->ifa_name) == "lo")
                            {
                                // the loopback device doesn't set a broadcast address but 127.255.255.255 can be used instead
                                items.push_back("127.255.255.255");
                            }
                            else
                            {
                                char str[INET_ADDRSTRLEN] {};

                                inet_ntop(AF_INET, &ip4addr->sin_addr, str, INET_ADDRSTRLEN);
                                items.push_back(str);
                            }
                        }
                    }
                }
            }
#else
            LOGERROR("Unimplemented");
#endif
            return items;
        }

    } // namespace net
} // namespace cqp
