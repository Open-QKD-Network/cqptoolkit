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
#include "TCPServerTunnel.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include <thread>

namespace cqp
{

    namespace tunnels
    {
        TCPServerTunnel::TCPServerTunnel(const URI& listenAddress) :
            net::Server(listenAddress)
        {

            LOGINFO("Waiting for connection on " + listenAddress.ToString());
            acceptorThread = std::thread(&TCPServerTunnel::DoAccept, this);
        }

        void TCPServerTunnel::DoAccept()
        {
            using namespace std::chrono;

            SetReceiveTimeout(milliseconds(1000));
            bool connected = false;

            do
            {
                SetKeepAlive(true);
                clientSock = AcceptConnection();
                if(clientSock)
                {
                    clientSock->SetKeepAlive(true);
                    clientSock->SetReceiveTimeout(milliseconds(3000));
                    ready = true;
                    readyCv.notify_all();
                    LOGINFO("Connection received.");
                    Close();
                    connected = true;
                }
            }
            while(keepGoing && !connected);
        }

        TCPServerTunnel::~TCPServerTunnel()
        {
            keepGoing = false;
            acceptorThread.join();
            Close();
        }

        bool TCPServerTunnel::Read(void* data, size_t length, size_t& bytesReceived)
        {
            return clientSock->Read(data, length, bytesReceived);
        }

        bool TCPServerTunnel::Write(const void* data, size_t length)
        {
            return clientSock->Write(data, length);
        }

    } // namespace tunnels

} // namespace cqp

