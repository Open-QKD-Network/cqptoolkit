/*!
* @file
* @brief EthTun
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "EthTap.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "Algorithms/Net/Devices/Device.h"
#if defined(__linux)
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <linux/if.h>
    #include <linux/if_tun.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/capability.h>
    #include <ifaddrs.h>
#endif

namespace cqp
{

    namespace tunnels
    {
        /// The device which tells the kernel to create a tun device
        const char clonedev[] = "/dev/net/tun";

        bool EthTap::SetPersist(bool on)
        {
            bool result = false;
            if(handle >= 0 && ioctl(handle, TUNSETPERSIST, on) >= 0)
            {
                result = true;
                LOGDEBUG("Set " + name + " persist to " + std::to_string(on));
            }
            else
            {
                LOGERROR("Failed to set " + name + " persist to " + std::to_string(on));
            }

            return result;
        }

        bool EthTap::SetOwner(int user, int group)
        {
            bool result = handle >= 0;
            if(result && user != -1)
            {
                result &= ioctl(handle, TUNSETOWNER, user) >= 0;
            }

            if(result && group != -1)
            {
                result &= ioctl(handle, TUNSETGROUP, group) >= 0;
            }

            if(!result)
            {
                LOGERROR("Failed to set " + name + " owner/group to " + std::to_string(user) + "/" + std::to_string(group));
            }

            return result;
        }

        EthTap::~EthTap()
        {
            net::Device::Down(name);
            close(handle);
        }

        bool EthTap::Read(void* data, size_t length, size_t& bytesReceived)
        {
            return Socket::Read(data, length, bytesReceived);
        }

        bool EthTap::Write(const void* data, size_t length)
        {
            return Socket::Write(data, length);
        }

        std::vector<EthTap::DeviceDetails> EthTap::FindDevices()
        {
            std::vector<EthTap::DeviceDetails> result;
#if defined (__linux)
            using namespace std;
            struct ifaddrs* ifaddr = nullptr;
            if(::getifaddrs(&ifaddr) != -1)
            {
                for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
                {
                    const unsigned int tapFlags = IFF_TUN | IFF_TAP;

                    if(ifa && (ifa->ifa_addr || (ifa->ifa_flags & tapFlags)))
                    {
                        // found an Ethernet like device
                        DeviceDetails foundDevice;
                        bool isValid = false;

                        foundDevice.name = ifa->ifa_name;

                        if(ifa->ifa_flags & IFF_TAP)
                        {
                            foundDevice.mode = Mode::Tap;
                            isValid = true;
                        }
                        else if(ifa->ifa_flags & IFF_TUN)
                        {
                            foundDevice.mode = Mode::Tun;
                            isValid = true;
                        }

                        if(ifa->ifa_addr)
                        {
                            net::SocketAddress address;
                            address.ip.FromStruct(*ifa->ifa_addr);
                        }

                        if(isValid)
                        {
                            result.push_back(foundDevice);
                        }
                    }
                    else
                    {
                        LOGWARN("Unknown device");
                    }
                }
            }
#else
            LOGERROR("Unimplemented");
#endif
            return result;
        }

        EthTap::EthTap(const std::string& deviceName, Mode mode, const std::string& address, const std::string& netMask)
        {
            LOGDEBUG("Creating device with name:" + deviceName + " setting ip to:" + address + "/" + netMask);

            handle = open(clonedev, O_RDWR);
            if(handle >= 0)
            {
                struct ifreq ifr {};
                /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
                 *        IFF_TAP   - TAP device
                 *
                 *        IFF_NO_PI - Do not provide packet information
                 */
                // use tap to ack like an Ethernet switch, all Ethernet packets are handled
                ifr.ifr_flags = IFF_NO_PI;
                switch(mode)
                {
                case Mode::Tun:
                    ifr.ifr_ifru.ifru_flags |= IFF_TUN;
                    break;

                case Mode::Tap:
                    ifr.ifr_ifru.ifru_flags |= IFF_TAP;
                    break;
                }

                if(!deviceName.empty())
                {
                    LOGDEBUG("Forcing name to: " + deviceName);
                    deviceName.copy(ifr.ifr_name, sizeof(ifr.ifr_name));
                }

                if(ioctl(handle, TUNSETIFF, &ifr) < 0)
                {
                    LOGERROR("Failed to setup tunnel");
                    LOGERROR(::strerror(errno));
                    close(handle);
                    handle = -1;
                }
                else
                {

                    // get the real name of the device
                    name = ifr.ifr_name;
                    LOGINFO("Created tun device " + name);
                    // TODO: use multi queue interface to speed up data transfer, see https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/networking/tuntap.txt?id=HEAD
                    bufferSize = static_cast<size_t>(ifr.ifr_ifru.ifru_mtu);

                    net::Device::SetAddress(name, address, netMask);
                    net::Device::Up(name);
                    ready = true;
                    readyCv.notify_all();
                }
            }
        }

        EthTap* EthTap::Create(const URI& uri)
        {
            Mode mode = Mode::Tap;
            if(uri.GetScheme() == Params::mode_tun)
            {
                mode = Mode::Tun;
            }
            return new EthTap(uri[Params::name], mode, uri.GetHost(), uri[Params::netmask]);
        }

    } // namespace tunnels
} // namespace cqp

