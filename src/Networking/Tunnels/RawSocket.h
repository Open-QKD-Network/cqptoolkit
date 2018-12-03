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
#pragma once
#include "CQPToolkit/Util/SecFileBuff.h"
#include "Networking/Tunnels/DeviceIO.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "Algorithms/Util/Strings.h"
#include "Networking/networking_export.h"

namespace cqp
{

    namespace tunnels
    {
        namespace RawSocketParams
        {
#if !defined(DOXYGEN)
    /// flag for promiscuous mode
    /// Values: true, false;
    NAMEDSTRING(prom);
    /// level of data captured
    /// Values: tcp, ip, eth
    NAMEDSTRING(level);
    NAMEDSTRING(tcp);
    NAMEDSTRING(ip);
    NAMEDSTRING(eth);
    NAMEDSTRING(name);
    NAMEDSTRING(netmask);
#endif
        }

        /**
         * @brief The RawSocket class
         * Read and write raw packets that hit an interface
         * @see https://en.wikipedia.org/wiki/Raw_socket
         */
        class NETWORKING_EXPORT RawSocket : public DeviceIO, public net::Socket
        {
        public:
            /// The protocol level at which to capture
            /// This dictates which headers are kept
            enum class Level
            {
                /// Kernel handles Ethernet and IP level, we see tcp
                datagram,
                /// Kernel handles Ethernet, we see IP level
                ip,
                /// Kernel handles nothing, we see Ethernet packets
                eth
            };

            /**
             * @brief Create
             * @param device Ethernet device name eg: "eth0"
             * @param level
             * @param promiscuous
             * @param address
             * @param netmask
             * @return The socket
             */
            static RawSocket* Create(const std::string& device, Level level, bool promiscuous, const std::string& address = "", const std::string& netmask = "");

            /**
             * @brief Create
             * @param uri
             * @return A raw socket configured fom uri
             */
            static RawSocket* Create(const URI& uri);

            /**
             * @brief ~RawSocket
             * Destructor
             */
            virtual ~RawSocket() override;

            ///@{
            /// @name DeviceIO methods

            /// @copydoc DeviceIO::Read
            bool Read(void* data, size_t length, size_t& bytesReceived) override;
            /// @copydoc DeviceIO::Write
            bool Write(const void* data, size_t length) override;

            /// @}

            /// @{
            /// @name net::Socket methods

            /// @copydoc net::Socket::Close
            void Close() override;
        protected:
            /// size of internal buffer
            size_t bufferSize = 0;
            /// name of the physical device
            std::string deviceName;

            RawSocket() {}
        };
    } // namespace tunnels

} // namespace cqp


