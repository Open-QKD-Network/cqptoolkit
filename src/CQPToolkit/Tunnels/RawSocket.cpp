/*!
* @file
* @brief RawSocket
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 11/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "RawSocket.h"
#include "Util/Logger.h"
#include <cstring>
#include "CQPToolkit/Net/Device.h"
#if defined(__linux)
    #include <sys/types.h>
    #include <ifaddrs.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <linux/if_packet.h>
    #include <linux/if_ether.h>
    #include <linux/if_arp.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif

namespace cqp
{

    namespace tunnels
    {
        RawSocket* RawSocket::Create(const std::string& device, Level level, bool promiscuous,
                                     const std::string& address, const std::string& netmask)
        {
            RawSocket* self = new RawSocket();
            bool result = false;
            self->deviceName = device;

            int domain = 0;
            int packetType = 0;
            uint16_t protocol = 0;

            switch (level)
            {
            case Level::datagram:
                domain = PF_PACKET;
                packetType = SOCK_DGRAM;
                protocol = ETH_P_IP;
                break;
            case Level::ip:
                domain = PF_PACKET;
                packetType = SOCK_RAW;
                protocol = ETH_P_IP;
                break;
            case Level::eth:
                domain = PF_PACKET;
                packetType = SOCK_RAW;
                protocol = ETH_P_ALL;
                break;
            }

            // create a raw socket to capture and send ethernet packets
            self->handle = socket(domain, packetType, htons(protocol));
            if(self->handle < 0)
            {
                LOGERROR("Failed to create raw socket: " + std::string(strerror(errno)));
            }
            else
            {
                struct ifreq ifaceFlags {};
                memset(&ifaceFlags, 0, sizeof(ifaceFlags));

                /* Set the device to use */
                device.copy(ifaceFlags.ifr_name, IFNAMSIZ);

                // put the device into promiscuous mode to collect all data that hits the device
                // see https://stackoverflow.com/questions/114804/reading-from-a-promiscuous-network-device
                if(ioctl(self->handle, SIOCGIFFLAGS, &ifaceFlags, sizeof(ifaceFlags)) == -1)
                {
                    result = false;
                    LOGERROR("Failed to get socket flags");
                }
                else
                {
                    LOGTRACE("Got current device flags");
                    result = true;
                    /* Set the old flags plus the IFF_PROMISC flag */
                    if(promiscuous)
                    {
                        LOGTRACE("Setting promiscuous flag");
                        ifaceFlags.ifr_flags |= IFF_PROMISC;
                        if (ioctl (self->handle, SIOCSIFFLAGS, &ifaceFlags, sizeof(ifaceFlags)) == -1)
                        {
                            LOGERROR("Could not set flag IFF_PROMISC");
                            result = false;
                        }
                    }

                    int reuse = 1;
                    if (setsockopt(self->handle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) == -1)
                    {
                        LOGERROR("Failed to set socket reuse");
                        result = false;
                    }
                }

                if(result)
                {
                    LOGTRACE("Binding to device");
                    struct ifreq binddev {};
                    memset(&binddev, 0, sizeof(binddev));
                    device.copy(binddev.ifr_name, IFNAMSIZ);

                    // bind to a specific device
                    //result = setsockopt(self->rawSock, SOL_SOCKET, SO_BINDTODEVICE, &binddev, sizeof(binddev)) == 0;
                    struct ifreq ifid = {};
                    device.copy(ifid.ifr_name, IFNAMSIZ);
                    if(ioctl(self->handle, SIOCGIFINDEX, &ifid, sizeof(ifid)) == -1)
                    {
                        LOGERROR("Failed to get interface id");
                    }
                    else
                    {
                        struct sockaddr_ll sll = {};

                        sll.sll_family = AF_PACKET;
                        sll.sll_ifindex = ifid.ifr_ifindex;
                        sll.sll_protocol = htons(protocol);
                        if(bind(self->handle, reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll)) == 0)
                        {
                            LOGTRACE("Bind successful");
                        }
                        else
                        {
                            result = false;
                            LOGERROR("Bind failed: " + ::strerror(errno));
                        }
                    }

                    struct ifreq devmtu {};
                    memset(&devmtu, 0, sizeof(devmtu));
                    device.copy(devmtu.ifr_name, IFNAMSIZ);

                    LOGTRACE("Getting device mtu");
                    if(ioctl(self->handle, SIOCGIFMTU, &devmtu, sizeof(ifaceFlags)) == 0 && devmtu.ifr_ifru.ifru_mtu > 0)
                    {
                        self->bufferSize = static_cast<size_t>(ifaceFlags.ifr_ifru.ifru_mtu);
                        LOGTRACE("MTU=" + std::to_string(self->bufferSize));
                    }
                }
            }

            if(result)
            {

                LOGTRACE("Setting device address");
                result = net::Device::SetAddress(device, address, netmask);
                LOGTRACE("Bringing device up");
                result &= net::Device::Up(device);

                if(result)
                {
                    LOGTRACE("Device ready");
                    self->ready = true;
                    self->readyCv.notify_all();
                }
            }

            if(!result)
            {
                LOGERROR("Raw socket NOT initialised");
                delete(self);
                self = nullptr;
            }
            return self;
        }

        RawSocket* RawSocket::Create(const URI& uri)
        {
            Level level = Level::datagram;
            bool prom = false;

            std::string levelStr;
            if(uri.GetFirstParameter(RawSocketParams::level, levelStr))
            {
                if(levelStr == RawSocketParams::eth)
                {
                    level = Level::eth;
                }
                else if(levelStr == RawSocketParams::tcp)
                {
                    level = Level::datagram;
                }
                else if(levelStr == RawSocketParams::ip)
                {
                    level = Level::ip;
                }
                else
                {
                    LOGERROR("Unkown level value: " + levelStr);
                }
            }

            uri.GetFirstParameter(RawSocketParams::prom, prom);

            return Create(uri[RawSocketParams::name], level, prom, uri.GetHost(), uri[RawSocketParams::netmask]);
        }

        RawSocket::~RawSocket()
        {
            RawSocket::Close();
        }

        bool RawSocket::Read(void* data, size_t length, size_t& bytesReceived)
        {
            return Socket::Read(data, length, bytesReceived);
        }

        bool RawSocket::Write(const void* data, size_t length)
        {
            return Socket::Write(data, length);
        }

        void RawSocket::Close()
        {
            Socket::Close();
            if(!net::Device::Down(deviceName))
            {
                LOGERROR("Failed to bring " + deviceName + " down");
            }
        }

    } // namespace tunnels
} // namespace cqp
