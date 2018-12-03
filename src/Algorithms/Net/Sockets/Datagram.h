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
#pragma once
#include "Algorithms/Net/Sockets/Socket.h"
#include "Algorithms/algorithms_export.h"
#include <chrono>

namespace cqp
{
    namespace net
    {

        /**
         * @brief The Datagram class
         * Access to UDP sockets
         */
        class ALGORITHMS_EXPORT Datagram : public Socket
        {
        public:
            /**
             * @brief Datagram
             * Constructor
             */
            Datagram();

            /**
             * @brief Datagram
             * Construct with bind address
             * @param bindAddress Address to bind to
             * @param sourcePort Port to bind to
             */
            Datagram(const std::string& bindAddress, uint16_t sourcePort = 0);

            /**
             * @brief SendTo
             * Send data
             * @param data Data to send
             * @param length length of data
             * @param destination address to send to
             * @return true on success
             */
            bool SendTo(const void* data, size_t length, const SocketAddress& destination);

            /**
             * @brief ReceiveFrom
             * @param[out] data Received data
             * @param[in] length Maximum data to read
             * @param[out] bytesReceived bytes actually read
             * @param[out] sender address of sender
             * @return true on success
             */
            bool ReceiveFrom(void* data, size_t length, size_t& bytesReceived, SocketAddress& sender);

        };

    } // namespace net
} // namespace cqp


