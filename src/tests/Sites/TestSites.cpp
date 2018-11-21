/*!
* @file
* @brief TestSites
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 04/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestSites.h"
#include "CQPToolkit/SDN/NetworkManagerDummy.h"
#include "CQPToolkit/KeyGen/SiteAgent.h"
#include "CQPToolkit/Drivers/DeviceFactory.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Drivers/DummyQKD.h"
#include <thread>

namespace cqp
{
    namespace tests
    {
        using ::testing::_;
        using namespace ::testing;

        class MyNetMan : public NetworkManagerDummy
        {


            // Service interface
        public:
            grpc::Status RegisterSite(grpc::ServerContext*, const remote::Site* request, google::protobuf::Empty*) override
            {
                // bounce to a simpler mock method
                return OnRegistered(request);
            }

            MOCK_METHOD1(OnRegistered, grpc::Status(const remote::Site* request));
        };

        /**
         * @test
         * @brief TEST
         */
        TEST(SiteTest, MultiHop)
        {
            using namespace std;
            using namespace grpc;

            DefaultLogger().SetOutputLevel(LogLevel::Debug);

            MyNetMan netman;
            int netmanPort = 0;
            netman.StartServer(netmanPort, grpc::InsecureServerCredentials());

            ASSERT_NE(netmanPort, 0) << "Failed to start netman";

            DummyQKD::RegisterWithFactory();

            // setup default settings
            remote::SiteAgentConfig config1;
            config1.set_name("Site1");
            config1.set_netmanuri("localhost:" + std::to_string(netmanPort));
            // devices for both directions
            config1.mutable_deviceurls()->Add("dummyqkd:///?side=alice&switchname=1234&switchport=1a");
            config1.mutable_deviceurls()->Add("dummyqkd:///?side=bob&switchname=1234&switchport=1b");

            // copy the config to the other settings
            remote::SiteAgentConfig config2(config1);
            config2.set_name("Site2");

            // copy the config to the other settings
            remote::SiteAgentConfig config3(config1);
            config3.set_name("Site3");

            // create the site agents
            SiteAgent site1(config1);
            SiteAgent site2(config2);
            SiteAgent site3(config3);

            bool reg1 = false;
            bool reg2 = false;
            bool reg3 = false;

            LOGINFO("Site1 = " + site1.GetConnectionAddress());
            LOGINFO("Site2 = " + site2.GetConnectionAddress());
            LOGINFO("Site3 = " + site3.GetConnectionAddress());

            // expectg a registration, but do nothing
            EXPECT_CALL(netman, OnRegistered(_)).WillOnce(Invoke([&](const remote::Site*)
            {
                LOGDEBUG("Register 1 called");
                reg1 = true;
                return grpc::Status();
            }));

            // do registration
            site1.RegisterWithNetMan();

            ASSERT_TRUE(reg1) << "Registration 1 failed";

            // expect second registration, start 1 - 2
            EXPECT_CALL(netman, OnRegistered(_)).WillOnce(Invoke([&](const remote::Site*) -> grpc::Status
            {
                LOGTRACE("Register 2 called");
                auto channel = grpc::CreateChannel(site2.GetConnectionAddress(), grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                auto site2Stub = remote::ISiteAgent::NewStub(channel);
                remote::PhysicalPath request;
                google::protobuf::Empty response;

                auto hop = request.mutable_hops()->Add();
                // specifiy the hop from site 1 alice to site 2 bob
                // To create a curve ball later, start it backwards send 2-1 to 2.
                // When 1 - 2, 2 - 3 is sent later, it should be able to cope
                hop->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config2.deviceurls(1)));
                hop->mutable_first()->set_site(site2.GetConnectionAddress());

                hop->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config1.deviceurls(0)));
                hop->mutable_second()->set_site(site1.GetConnectionAddress());

                LOGTRACE("Calling StartNode 2 - 1 on 2");
                reg2 = LogStatus(site2Stub->StartNode(&ctx, request, &response)).ok();

                LOGTRACE("Register 2 Finished");
                return grpc::Status();
            }));

            // do registration
            site2.RegisterWithNetMan();

            ASSERT_TRUE(reg2) << "Registration 2 failed";

            // expect third registration, start 1 - 2, 2-3
            EXPECT_CALL(netman, OnRegistered(_)).WillOnce(Invoke([&](const remote::Site*)
            {
                LOGTRACE("Register 3 called");
                auto channel = grpc::CreateChannel(site1.GetConnectionAddress(), grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                auto site1Stub = remote::ISiteAgent::NewStub(channel);
                remote::PhysicalPath request;
                google::protobuf::Empty response;

                auto hop1 = request.mutable_hops()->Add();
                // specifiy the hop from site 1 alice to site 2 bob
                hop1->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config1.deviceurls(0)));
                hop1->mutable_first()->set_site(site1.GetConnectionAddress());

                hop1->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config2.deviceurls(1)));
                hop1->mutable_second()->set_site(site2.GetConnectionAddress());

                auto hop2 = request.mutable_hops()->Add();
                // specifiy the hop from site 1 alice to site 2 bob
                hop2->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config2.deviceurls(1)));
                hop2->mutable_first()->set_site(site2.GetConnectionAddress());

                hop2->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config3.deviceurls(0)));
                hop2->mutable_second()->set_site(site3.GetConnectionAddress());

                LOGTRACE("Calling StartNode 1 - 2, 2 - 3");
                reg3 = LogStatus(site1Stub->StartNode(&ctx, request, &response)).ok();

                LOGTRACE("Register 3 finished");
                return grpc::Status();
            }));

            // do registration
            site3.RegisterWithNetMan();

            ASSERT_TRUE(reg3) << "Registration 3 failed";

            for(const std::string& from :
                    {
                        site1.GetConnectionAddress(), site2.GetConnectionAddress(), site3.GetConnectionAddress()
                    })
            {
                auto channel = grpc::CreateChannel(from, grpc::InsecureChannelCredentials());
                auto site1Stub = remote::IKey::NewStub(channel);
                grpc::ClientContext ctx;
                google::protobuf::Empty request;
                remote::SiteList response;
                LOGTRACE("Getting list of key stores from " + from);
                ASSERT_TRUE(site1Stub->GetKeyStores(&ctx, request, &response).ok());
                ASSERT_EQ(response.urls().size(), 2) << "Wrong number of key stores";
                ASSERT_NE(response.urls(0), response.urls(1)) << "Same keystore twice";

                // try and get a key from each destination
                for(const auto& ksAddr : response.urls())
                {
                    LOGDEBUG("Getting key from " + from + " to " + ksAddr);
                    grpc::ClientContext keyCtx;
                    remote::KeyRequest keyRequest;
                    remote::SharedKey keyResponse;
                    keyRequest.set_siteto(ksAddr);
                    ASSERT_TRUE(site1Stub->GetSharedKey(&keyCtx, keyRequest, &keyResponse).ok());
                    ASSERT_GT(keyResponse.keyid(), 0);
                    ASSERT_GT(keyResponse.keyvalue().size(), 0);
                }
            }

            {
                LOGTRACE("Stopping site 1-2, 2-3...");
                auto channel = grpc::CreateChannel(site1.GetConnectionAddress(), grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                auto site1Stub = remote::ISiteAgent::NewStub(channel);
                remote::PhysicalPath request;
                google::protobuf::Empty response;

                auto hop1 = request.mutable_hops()->Add();
                // specifiy the hop from site 1 alice to site 2 bob
                hop1->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config1.deviceurls(0)));
                hop1->mutable_first()->set_site(site1.GetConnectionAddress());

                hop1->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config2.deviceurls(1)));
                hop1->mutable_second()->set_site(site2.GetConnectionAddress());

                auto hop2 = request.mutable_hops()->Add();
                // specifiy the hop from site 1 alice to site 2 bob
                hop2->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config2.deviceurls(1)));
                hop2->mutable_first()->set_site(site2.GetConnectionAddress());

                hop2->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(config3.deviceurls(0)));
                hop2->mutable_second()->set_site(site3.GetConnectionAddress());

                ASSERT_TRUE(LogStatus(site1Stub->EndKeyExchange(&ctx, request, &response)).ok());
            }

        }
    } // namespace tests
} // namespace cqp
