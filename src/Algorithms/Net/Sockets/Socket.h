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
#pragma once
#include "Algorithms/algorithms_export.h"
#include <chrono>
#include <array>
#include <memory>

struct sockaddr;
struct sockaddr_storage;

namespace cqp
{
    namespace net
    {

        /**
         * @brief The IPAddress struct
         * Holds an ip address
         */
        struct ALGORITHMS_EXPORT IPAddress
        {
            /// type for ipv4
            using IPv4Type = std::array<unsigned char, 4>;
            /// type for ipv6
            using IPv6Type = std::array<unsigned char, 16>;
            /// Type of ip stored
            bool isIPv4 = true;

            /// storage for address
            union
            {
                /// IPv4 storage
                IPv4Type ip4;
                /// IPv6 storage
                IPv6Type ip6;
            };

            /**
             * @brief IPAddress
             * Constructor
             */
            IPAddress();

            /**
             * @brief IPAddress
             * @param input
             */
            explicit IPAddress(const std::string& input);

            /**
             * @brief ToString
             * @return ip as a string
             */
            std::string ToString() const;

            /**
             * @brief FromStruct
             * @param addr
             */
            void FromStruct(const struct ::sockaddr_storage& addr);

            /**
             * @brief FromStruct
             * @param addr
             */
            void FromStruct(const struct ::sockaddr& addr);

            /**
             * @brief ToStruct
             * Converts an ip into a system structure
             * @param[out] size The size of the created structure
             * @return A pointer to the constructed address
             */
            std::unique_ptr<const struct ::sockaddr> ToStruct(unsigned int& size) const;

            /**
             * @brief IsNull
             * @return true if ip is all 0
             */
            bool IsNull() const;
        };

        /**
         * @brief The SocketAddress struct
         * Holds an ip and port
         */
        struct ALGORITHMS_EXPORT SocketAddress
        {
            /// the ip address
            IPAddress ip;
            /// the port number
            uint16_t port = 0;

            /**
             * @brief SocketAddress
             * Constructor
             */
            SocketAddress() = default;

            /**
             * @brief SocketAddress
             * Construct from string
             * @param value address in hostname:port format
             */
            SocketAddress(const std::string& value);

            /**
             * @brief SocketAddress
             * Construct from string
             * @param value address in hostname:port format
             */
            SocketAddress(const char* value) : SocketAddress(std::string(value)) {}

            /**
             * @brief ToString
             * @return address and string
             */
            std::string ToString() const;

            /**
             * @brief ToString
             * @return address and string
             */
            operator std::string() const
            {
                return ToString();
            }

            /**
             * @brief ToStruct
             * Converts an ip into a system structure
             * @param[out] size The size of the created structure
             * @return A pointer to the constructed address
             */
            std::unique_ptr<const struct ::sockaddr> ToStruct(unsigned int& size) const;

            /**
             * @brief FromStruct
             * @param addr
             */
            void FromStruct(const struct ::sockaddr_storage& addr);
        };

        /**
         * @brief operator ==
         * @param lhs
         * @param rhs
         * @return true if the ip addresses match
         */
        ALGORITHMS_EXPORT inline bool operator ==(const IPAddress& lhs, const IPAddress& rhs)
        {
            bool result = false;
            if(lhs.isIPv4 && rhs.isIPv4)
            {
                result = lhs.ip4 == rhs.ip4;
            }
            else if(!lhs.isIPv4 && !rhs.isIPv4)
            {
                result = lhs.ip6 == rhs.ip6;
            }
            return result;
        }
        /**
         * @brief operator !=
         * @param lhs
         * @param rhs
         * @return return true if the ip addresses don't match
         */
        ALGORITHMS_EXPORT inline bool operator !=(const IPAddress& lhs, const IPAddress& rhs)
        {
            return !(lhs == rhs);
        }

        /**
         * @brief operator ==
         * @param lhs
         * @param rhs
         * @return true if the addresses match
         */
        ALGORITHMS_EXPORT inline bool operator ==(const SocketAddress& lhs, const SocketAddress& rhs)
        {
            return lhs.ip == rhs.ip && lhs.port == rhs.port;
        }
        /**
         * @brief operator !=
         * @param lhs
         * @param rhs
         * @return return true if the address don't match
         */
        ALGORITHMS_EXPORT inline bool operator !=(const SocketAddress& lhs, const SocketAddress& rhs)
        {
            return !(lhs == rhs);
        }

        /**
         * @brief The Socket class
         * Provides access to network sockets
         */
        class ALGORITHMS_EXPORT Socket
        {
        public:
            /**
             * @brief Socket
             * Constructor
             */
            Socket() = default;

            /**
             * @brief Bind
             * link this socket to a specific address, instead of one provided by the os
             * @param address
             * @return true on success
             */
            bool Bind(const SocketAddress& address = SocketAddress());

            /**
             * @brief SetReceiveTimeout
             * @param timeout Duration after which a receive will fail
             * @return true on success
             */
            bool SetReceiveTimeout(std::chrono::milliseconds timeout);

            /**
             * @brief Close
             * Close the socket
             */
            virtual void Close();

            /**
             * @brief ~Socket
             */
            virtual ~Socket();

            /**
             * @brief GetAddress
             * @return The address which this socket is bond to
             */
            SocketAddress GetAddress() const;

            /**
             * @brief SetBlocking
             * @param active Whether the socket should be blocking
             * @return true on success
             */
            bool SetBlocking(bool active);

            /**
             * @brief IsBlocking
             * @return true if the socket is in blocking mode
             */
            bool IsBlocking();

            /**
             * @brief Read
             * Read data from the socket
             * @param[in,out] data destination for the bytes
             * @param[in] length The size of data
             * @param[out] bytesReceived The number of bytes read
             * @return true on success
             */
            bool Read(void* data, size_t length, size_t& bytesReceived);

            /**
             * @brief Write
             * Send data over the socket
             * @param data Data to send
             * @param length Number of bytes to send
             * @return true on success
             */
            bool Write(const void* data, size_t length);

        protected:
            /// The device handle
            int handle = 0;
        }; // class Socket

    } // namespace net
} // namespace cqp


