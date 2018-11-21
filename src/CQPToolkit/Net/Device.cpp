/*!
* @file
* @brief Device
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 10/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Device.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Util/Logger.h"

#include <string.h>
#if defined(__linux)
    #include <linux/if.h>
    #include <linux/in.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif
namespace cqp
{
    namespace net
    {

        Device::Device()
        {

        }

        bool Device::SetAddress(const std::string& devName, const std::string& address, const std::string& netmask)
        {
            bool result = address.empty();
            if(!address.empty())
            {
                LOGTRACE("Setting ip address of " + devName + " to " + address + "/" + netmask);
                net::IPAddress ip(address);
                unsigned int addrSize = 0;
                struct ifreq ipStruct {};

                int udpSock = 0;
                if(ip.isIPv4)
                {
                    udpSock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
                }
                else
                {
                    udpSock = ::socket(PF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
                }

                if(udpSock > 0)
                {
                    devName.copy(ipStruct.ifr_ifrn.ifrn_name, IFNAMSIZ);
                    ipStruct.ifr_ifru.ifru_addr = *ip.ToStruct(addrSize);

                    if(::ioctl(udpSock, SIOCSIFADDR, &ipStruct) < 0)
                    {
                        LOGERROR("Failed to set ip to " + ip.ToString());
                        LOGERROR(::strerror(errno));
                    }
                    else
                    {
                        result = true;
                    }

                    if(!netmask.empty())
                    {
                        net::IPAddress net(netmask);
                        ipStruct.ifr_ifru.ifru_addr = *net.ToStruct(addrSize);

                        if(::ioctl(udpSock, SIOCSIFNETMASK, &ipStruct) < 0)
                        {
                            LOGERROR("Failed to set netmask to " + net.ToString());
                            LOGERROR(::strerror(errno));
                            result = false;
                        }

                    }

                    ::close(udpSock);
                }
                else
                {
                    LOGERROR("Failed to open udp socket on new device");
                    LOGERROR(::strerror(errno));
                }
            } // if address
            return result;
        }

        bool Device::Up(const std::string& devName)
        {
            return SetRunState(devName, true);
        }

        bool Device::Down(const std::string& devName)
        {
            return SetRunState(devName, false);
        }

        bool Device::SetRunState(const std::string& devName, bool up)
        {
            bool result = false;
            int udpSock = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

            if(udpSock > 0)
            {
                struct ifreq ipStruct {};

                LOGTRACE("Getting device flags");
                devName.copy(ipStruct.ifr_ifrn.ifrn_name, IFNAMSIZ);
                if(::ioctl(udpSock, SIOCGIFFLAGS, &ipStruct) < 0 )
                {
                    LOGERROR("Failed to get interface flags");
                    LOGERROR(::strerror(errno));
                }
                else
                {
                    if(up)
                    {
                        LOGTRACE("Bringing " + devName + " up.");
                        // bring the interface up
                        ipStruct.ifr_ifru.ifru_flags |= IFF_UP | IFF_RUNNING;
                    }
                    else
                    {
                        LOGTRACE("Bringing " + devName + " down.");
                        // bring the interface up
                        ipStruct.ifr_ifru.ifru_flags &= ~IFF_UP;
                    }
                    if(ioctl(udpSock, SIOCSIFFLAGS, &ipStruct) < 0)
                    {
                        LOGERROR("Failed to set interface up");
                        LOGERROR(::strerror(errno));
                    }
                    else
                    {
                        result = true;
                    }
                }
                ::close(udpSock);
            }
            else
            {
                LOGERROR("Failed to open udp socket on new device");
                LOGERROR(::strerror(errno));
            }
            return result;
        }

    } // namespace net
} // namespace cqp
