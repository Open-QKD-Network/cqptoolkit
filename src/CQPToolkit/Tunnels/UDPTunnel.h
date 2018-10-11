/*!
* @file
* @brief UDPTunnel
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <iostream>
#include "CQPToolkit/Tunnels/DeviceIO.h"
#include "Util/URI.h"
#include "CQPToolkit/Net/Datagram.h"

namespace cqp
{
    namespace tunnels
    {
        /**
         * @brief The UDPTunnel class
         * basic io class which provides access to a udp port
         */
        class UDPTunnel : public virtual DeviceIO, protected net::Datagram
        {
        public:
            /**
             * @brief UDPTunnel
             * @param address
             */
            UDPTunnel(const URI& address);

            /// Distructor
            virtual ~UDPTunnel() override;

            ///@{
            /// @name DeviceIO methods

            /// @copydoc DeviceIO::Read
            bool Read(void* data, size_t length, size_t& bytesReceived) override;
            /// @copydoc DeviceIO::Write
            bool Write(const void* data, size_t length) override;

            /// @}
        protected:
        };

    } // namespace tunnels
} // namespace cqp


