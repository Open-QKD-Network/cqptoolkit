/*!
* @file
* @brief TunnelTests
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/8/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TunnelTests.h"
#include "Networking/Tunnels/Controller.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Random/RandomNumber.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Net/Sockets/Stream.h"
#include <gmock/gmock.h>

namespace cqp
{
    namespace tests
    {

        using namespace testing;

        TunnelTests::TunnelTests() :
            factory1(grpc::InsecureChannelCredentials()),
            factory2(grpc::InsecureChannelCredentials())
        {

            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Trace);

            {
                // create the server for second factory
                grpc::ServerBuilder builder;
                server1ListenPort = 0;
                builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &server1ListenPort);
                builder.RegisterService(static_cast<remote::IKeyFactory::Service*>(&factory1));
                builder.RegisterService(static_cast<remote::IKey::Service*>(&factory1));
                keyServer1 = builder.BuildAndStart();

                LOGDEBUG("Key Server 1 on 127.0.0.1:" + std::to_string(server1ListenPort));
                factory1.SetSiteAddress("127.0.0.1:" + std::to_string(server1ListenPort));
            }

            {
                // create the server for second factory
                grpc::ServerBuilder builder;
                server2ListenPort = 0;
                builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &server2ListenPort);
                builder.RegisterService(static_cast<remote::IKeyFactory::Service*>(&factory2));
                builder.RegisterService(static_cast<remote::IKey::Service*>(&factory2));
                keyServer2 = builder.BuildAndStart();

                LOGDEBUG("Key Server 2 on 127.0.0.1:" + std::to_string(server2ListenPort));
                factory2.SetSiteAddress("127.0.0.1:" + std::to_string(server2ListenPort));
            }

            SeedKeyStores();
        }

        void TunnelTests::SeedKeyStores()
        {
            auto ks1to2 = factory1.GetKeyStore("127.0.0.1:" + std::to_string(server2ListenPort));
            auto ks2to1 = factory2.GetKeyStore("127.0.0.1:" + std::to_string(server1ListenPort));

            const auto numKeys = 1000u;
            PSK tempKey;
            KeyList keys;
            keys.reserve(numKeys);
            RandomNumber rng;

            for(uint count = 0; count < numKeys; count++)
            {
                rng.RandomBytes(32, tempKey);
                keys.emplace_back(tempKey);
                tempKey.clear();
            }

            // seed the keystores
            ks1to2->OnKeyGeneration(std::make_unique<KeyList>(keys.begin(), keys.end()));
            ks2to1->OnKeyGeneration(std::make_unique<KeyList>(keys.begin(), keys.end()));
        }

        TEST_F(TunnelTests, TCPTun)
        {
            remote::tunnels::ControllerDetails details1;

            details1.set_name("testtun1");
            details1.set_localkeyfactoryuri("127.0.0.1:" + std::to_string(server1ListenPort));

            tunnels::Controller controller1(details1);

            {
                {
                    grpc::ServerContext ctx;
                    google::protobuf::Empty request;
                    ASSERT_TRUE(controller1.GetControllerSettings(&ctx, &request, &details1).ok());
                }

                // create the server
                grpc::ServerBuilder builder;
                int listenPort = 0;
                builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &listenPort);

                LOGTRACE("Registering services");
                // Register services
                builder.RegisterService(static_cast<remote::tunnels::ITunnelServer::Service*>(&controller1));
                // ^^^ Add new services here ^^^ //

                LOGTRACE("Starting tunnel server 1");
                farTunServer = builder.BuildAndStart();
                details1.set_connectionuri("127.0.0.1:" + std::to_string(listenPort));
                LOGDEBUG("Controller 1 server available: " + details1.connectionuri());
            }

            {
                remote::tunnels::ControllerDetails details2;
                details2.set_name("testtun2");
                details2.set_localkeyfactoryuri("127.0.0.1:" + std::to_string(server2ListenPort));

                remote::tunnels::Tunnel tunConfig;
                tunConfig.set_name("tcptun");
                tunConfig.mutable_keylifespan()->set_maxbytes(10);
                tunConfig.set_remotecontrolleruri(details1.connectionuri());
                tunConfig.mutable_startnode()->set_clientdataporturi("tcpsrv://localhost:8000");
                tunConfig.mutable_endnode()->set_clientdataporturi("tcpsrv://localhost:8001");

                tunConfig.set_remoteencryptedlistenaddress("127.0.0.1:0");

                details2.mutable_tunnels()->insert({tunConfig.name(), tunConfig});

                tunnels::Controller controller2(details2);
                google::protobuf::StringValue tunName;
                tunName.set_value(tunConfig.name());

                {
                    grpc::ServerContext ctx;
                    google::protobuf::Empty response;
                    ASSERT_TRUE(LogStatus(controller2.StartTunnel(&ctx, &tunName, &response)).ok());
                }

                net::Stream client1;
                net::Stream client2;

                ASSERT_TRUE(client1.Connect(net::SocketAddress("127.0.0.1:8000")));
                ASSERT_TRUE(client2.Connect(net::SocketAddress("127.0.0.1:8001")));

                const std::string message("Only two things are infinite, the universe and human stupidity, and I'm not sure about the former.");

                {
                    ASSERT_TRUE(client1.Write(message.c_str(), message.length()));
                    DataBlock buffer(message.length() + 1);
                    size_t numRead = 0;
                    ASSERT_TRUE(client2.Read(buffer.data(), buffer.size(), numRead));
                    ASSERT_EQ(numRead, message.length());
                    buffer.resize(numRead);
                    ASSERT_THAT(message, ElementsAreArray(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
                }
                //======//
                {
                    ASSERT_TRUE(client2.Write(message.c_str(), message.length()));
                    DataBlock buffer(message.length() + 1);
                    size_t numRead = 0;
                    ASSERT_TRUE(client1.Read(buffer.data(), buffer.size(), numRead));
                    ASSERT_EQ(numRead, message.length());
                    buffer.resize(numRead);
                    ASSERT_THAT(message, ElementsAreArray(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
                }

                client1.Close();
                client2.Close();
                {
                    grpc::ServerContext ctx;
                    google::protobuf::Empty response;
                    ASSERT_TRUE(LogStatus(controller2.StopTunnel(&ctx, &tunName, &response)).ok());
                }
            }
        }
    } // namespace tests
} // namespace cqp
