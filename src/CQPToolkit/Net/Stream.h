/*!
* @file
* @brief Stream
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Util/URI.h"

namespace cqp
{
    namespace net
    {
        class Server;

        /**
         * @brief The Stream class
         * TCP state-full socket
         */
        class CQPTOOLKIT_EXPORT Stream : public Socket
        {
        public:
            /**
             * @brief Stream
             * Constructor
             */
            Stream();
            /// Destructor
            ~Stream() = default;
            /**
             * @brief Connect
             * Connect to a server port
             * @param address host:port form address
             * @param timeout duration to wait before failing connection
             * @return true on connected successfully
             */
            bool Connect(const SocketAddress& address, std::chrono::milliseconds timeout = std::chrono::milliseconds(30000));
            /**
             * @brief SetKeepAlive
             * Change the keepalive state of the socket
             * Must have first called connect
             * @param active if true, keepalive packets will be sent
             * @return true if the setting was changed successfully
             */
            bool SetKeepAlive(bool active);

            /// give Server class access to this class
            friend Server;
        protected:
            /**
             * @brief Stream
             * @param fd raw file handle to construct from
             */
            explicit Stream(int fd);
        };

    } // namespace net
} // namespace cqp


