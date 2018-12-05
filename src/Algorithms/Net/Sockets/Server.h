/*!
* @file
* @brief Server
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Net/Sockets/Stream.h"
#include "Algorithms/algorithms_export.h"

namespace cqp
{
    namespace net
    {

        class Stream;

        /**
         * @brief The Server class
         * TCP acceptor
         */
        class ALGORITHMS_EXPORT Server : public Stream
        {
        public:
            /**
             * @brief Server
             * Constructor
             */
            Server() = default;
            /**
             * @brief Server
             * Construct and listen on address
             * @param listenAddress
             */
            explicit Server(const SocketAddress& listenAddress);
            /**
             * @brief Listen
             * Start listening on address
             * @param listenAddress
             */
            void Listen(const SocketAddress& listenAddress);
            /**
             * @brief AcceptConnection
             * Wait for connection from client
             * @return new client connection
             */
            std::shared_ptr<Stream> AcceptConnection();
        };

    } // namespace net
} // namespace cqp


