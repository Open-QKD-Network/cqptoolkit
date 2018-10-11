/*!
* @file
* @brief TCPServerTunnel
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/10/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>
#include "CQPToolkit/Tunnels/DeviceIO.h"
#include <thread>
#include "CQPToolkit/Net/Server.h"
#include "CQPToolkit/Net/Stream.h"

namespace cqp
{
    namespace tunnels
    {
        /**
         * @brief The TCPServerTunnel class
         * TCP listening socket as a data channel
         */
        class TCPServerTunnel : public virtual DeviceIO, protected net::Server
        {
        public:
            /**
             * @brief TCPServerTunnel
             * Constructor
             * @param listenAddress  The address of the server as a url
             */
            TCPServerTunnel(const URI& listenAddress);

            /// Distructor
            virtual ~TCPServerTunnel() override;

            ///@{
            /// @name DeviceIO methods

            /// @copydoc DeviceIO::Read
            bool Read(void* data, size_t length, size_t& bytesReceived) override;
            /// @copydoc DeviceIO::Write
            bool Write(const void* data, size_t length) override;

            /// @}
        protected:
            /**
             * @brief DoAccept
             * Wait for a client to connect
             */
            void DoAccept();
            /// socket created for a client
            std::shared_ptr<net::Stream> clientSock;
            /// thread for waiting for a client
            std::thread acceptorThread;
            /// for stopping the acceptor thread
            bool keepGoing = true;
        };
    }
} // namespace cqp
