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
#include "TCPTunnel.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include <thread>

namespace cqp
{

    namespace tunnels
    {
        TCPTunnel::TCPTunnel(const URI& connectAddress, std::chrono::milliseconds connectionTimeout, int attempts)
        {
            bool connected = false;
            using namespace std::chrono;

            do
            {
                LOGINFO("Connecting to " + connectAddress.ToString());
                try
                {
                    attempts--;
                    Connect(connectAddress, connectionTimeout);
                    LOGINFO("Connection received.");
                    SetKeepAlive(true);
                    SetReceiveTimeout(milliseconds(3000));
                    connected = true;
                }
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                    std::this_thread::sleep_for(milliseconds(500));
                }
            }
            while(!connected && attempts != 0);
        }

        TCPTunnel::~TCPTunnel()
        {
            Close();
        }

        bool TCPTunnel::Read(void* data, size_t length, size_t& bytesReceived)
        {
            return Stream::Read(data, length, bytesReceived);
        }

        bool TCPTunnel::Write(const void* data, size_t length)
        {
            return Stream::Write(data, length);
        }

    } // namespace tunnels

} // namespace cqp


