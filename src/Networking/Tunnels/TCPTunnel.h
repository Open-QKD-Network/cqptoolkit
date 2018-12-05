/*!
* @file
* @brief TCPTunnel
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/10/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/SecFileBuff.h"
#include "Networking/Tunnels/DeviceIO.h"
#include "Algorithms/Net/Sockets/Stream.h"
#include "Algorithms/Datatypes/URI.h"

namespace cqp
{

    namespace tunnels
    {
        /**
         * @brief The TCPTunnel class
         * A data stream class which connects to a TCP Server
         */
        class TCPTunnel : public virtual DeviceIO, protected net::Stream
        {
        public:
            /**
             * @brief TCPTunnel
             * Constructor
             * @param connectAddress The address of the server as a url
             * @param connectionTimeout Time to wait before failing to connect
             * @param attempts failed connection attempts before failing connection
             */
            TCPTunnel(const URI& connectAddress,
                      std::chrono::milliseconds connectionTimeout = std::chrono::milliseconds(30000),
                      int attempts = -1);
            virtual ~TCPTunnel() override;

            ///@{
            /// @name DeviceIO methods

            /// @copydoc DeviceIO::Read
            bool Read(void* data, size_t length, size_t& bytesReceived) override;
            /// @copydoc DeviceIO::Write
            bool Write(const void* data, size_t length) override;

            /// @}
        };
    }
} // namespace cqp


