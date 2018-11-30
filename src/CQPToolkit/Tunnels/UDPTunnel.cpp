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
#include "UDPTunnel.h"
#include "CQPAlgorithms/Logging/Logger.h"

namespace cqp
{
    namespace tunnels
    {

        UDPTunnel::UDPTunnel(const URI& address)
        {
            LOGINFO("listening on " + address.ToString());
            try
            {
                Bind(address);
                ready = true;
                readyCv.notify_all();
                LOGINFO("Connection ready.");
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }

        UDPTunnel::~UDPTunnel()
        {
            Close();
        }

        bool UDPTunnel::Read(void* data, size_t length, size_t& bytesReceived)
        {
            return Datagram::Read(data, length, bytesReceived);
        }

        bool UDPTunnel::Write(const void* data, size_t length)
        {
            return Datagram::Write(data, length);
        }

    } // namespace tunnels
} // namespace cqp

