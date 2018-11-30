/*!
* @file
* @brief AlignmentTestData
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestSockets.h"
#include <thread>
#include <chrono>
#include "CQPToolkit/Net/Datagram.h"
#include "CQPToolkit/Net/Server.h"
#include "CQPToolkit/Net/Stream.h"
#include <future>
#include <condition_variable>
#include "CQPToolkit/Util/URI.h"
#include "CQPToolkit/Tunnels/RawSocket.h"
#include "CQPAlgorithms/Logging/Logger.h"

namespace cqp
{
    namespace tests
    {

        TEST(Net, Datagram)
        {
            net::Datagram dg1("localhost");
            net::Datagram dg2("localhost");

            ASSERT_TRUE(dg2.SetReceiveTimeout(std::chrono::milliseconds(100)));
            URI dest = dg2.GetAddress();
            dest.SetHost("127.0.0.1");

            std::string sent = "daioudhvurnirger";
            char received[50] = {};
            ASSERT_TRUE(dg1.SendTo(sent.data(), sent.size(), dest));
            net::SocketAddress addr;
            size_t bytesReceived = 0;
            ASSERT_TRUE(dg2.ReceiveFrom(received, 50, bytesReceived, addr));
            ASSERT_EQ(bytesReceived, sent.size());
            ASSERT_EQ(sent, std::string(received));
            ASSERT_EQ(addr.ToString(), dg1.GetAddress().ToString());
        }

        TEST(Net, Stream)
        {
            std::mutex m;
            std::condition_variable cv;

            net::Server server("localhost");
            net::Stream client1;

            std::string sent = "daioudhvurnirger";
            char received[50] = {};
            size_t bytesReceived = 0;

            std::future<void> acceptor = std::async([&]()
            {
                std::shared_ptr<net::Stream> client2 = server.AcceptConnection();
                ASSERT_NE(client2, nullptr);
                ASSERT_TRUE(client2->Read(received, 50, bytesReceived));
                std::lock_guard<std::mutex> lock(m);
                cv.notify_one();
            });

            ASSERT_TRUE(client1.Connect(server.GetAddress()));
            ASSERT_TRUE(client1.Write(sent.data(), sent.size()));

            {
                std::unique_lock<std::mutex> lock(m);

                cv.wait(lock, [&]
                {
                    return bytesReceived > 0;
                });
            }

            ASSERT_EQ(bytesReceived, sent.size());
            ASSERT_EQ(sent, std::string(received));
        }

        TEST(Net, DISABLED_Raw)
        {
            DefaultLogger().SetOutputLevel(LogLevel::Trace);
            using tunnels::RawSocket;
            RawSocket* sock = nullptr;
            sock = RawSocket::Create("enp5s0u1u4", RawSocket::Level::eth, false);
            ASSERT_NE(sock, nullptr);
            ASSERT_TRUE(sock->WaitUntilReady());

            char buffer[1024] {};
            size_t recieved {};
            ASSERT_TRUE(sock->Read(&buffer, sizeof(buffer), recieved));
        }
    } // namespace tests
} // namespace cqp
