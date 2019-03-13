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
#include "KeyManagement/SDN/NetworkManagerDummy.h"
#include "KeyManagement/Sites/SiteAgent.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/QKDDevices/DummyQKD.h"
#include <thread>
#include "CQPToolkit/QKDDevices/DeviceUtils.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "Algorithms/Net/DNS.h"
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"

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

        struct SiteTestCollection
        {
            SiteTestCollection(remote::Side::Type side, const std::string& siteAgentAddress)
            {

                using namespace std;
                serverCreds = grpc::InsecureServerCredentials();
                LOGTRACE("Creating Device");
                device = make_shared<DummyQKD>(side, grpc::InsecureChannelCredentials());
                LOGTRACE("Creating adaptor");
                adaptor = make_shared<RemoteQKDDevice>(device, serverCreds);
                grpc::ServerBuilder builder;
                builder.RegisterService(adaptor.get());
                builder.AddListeningPort(std::string(net::AnyAddress) + ":0", serverCreds, &port);
                LOGTRACE("Starting server");
                server = builder.BuildAndStart();

                if(server)
                {
                    LOGINFO("Remote device control available on port " + std::to_string(port));
                    adaptor->SetControlAddress(net::GetHostname(true) + ":" + std::to_string(port));
                    if(!siteAgentAddress.empty())
                    {
                        LOGTRACE("Registering with site agent");
                        result = LogStatus(adaptor->RegisterWithSiteAgent(siteAgentAddress));
                    }
                } else {
                    LOGERROR("Failed to start server");
                }
            }

            ~SiteTestCollection()
            {
                adaptor.reset();
                device.reset();
                server.reset();
            }
            std::shared_ptr<grpc::ServerCredentials> serverCreds;
            std::shared_ptr<DummyQKD> device;
            std::shared_ptr<RemoteQKDDevice> adaptor;
            remote::SessionDetails sessionDetails;
            std::unique_ptr<grpc::Server> server;
            int port = 0;
            grpc::Status result;
        };

        struct SiteAgentBuilder
        {
            SiteAgentBuilder(const std::string& name, uint16_t port)
            {
                using namespace std;
                config.set_name(name);
                config.set_listenport(port);
                agent = make_shared<SiteAgent>(config);
            }

            remote::SiteAgentConfig config;
            std::shared_ptr<SiteAgent> agent;

        };

        TEST(SiteTest, Simple)
        {
            using namespace std;

            SiteAgentBuilder site1("Site1", 8001);
            // create site 1's device
            SiteTestCollection site1alice(remote::Side::Alice, site1.agent->GetConnectionAddress());
            ASSERT_TRUE(site1alice.result.ok());

            SiteAgentBuilder site2("Site2", 8002);
            SiteTestCollection site2bob(remote::Side::Bob, site2.agent->GetConnectionAddress());
            ASSERT_TRUE(site2bob.result.ok());

            // setup complete

            LOGINFO("Connecting to " + site2.agent->GetConnectionAddress());
            auto channel = grpc::CreateChannel(site2.agent->GetConnectionAddress(), grpc::InsecureChannelCredentials());
            grpc::ClientContext ctx;
            auto site2Stub = remote::ISiteAgent::NewStub(channel);
            remote::PhysicalPath request;
            google::protobuf::Empty response;

            auto hop = request.add_hops();
            hop->mutable_first()->set_site(site2.agent->GetConnectionAddress());
            hop->mutable_first()->set_deviceid(site2bob.device->GetDeviceDetails().id());
            hop->mutable_second()->set_site(site1.agent->GetConnectionAddress());
            hop->mutable_second()->set_deviceid(site1alice.device->GetDeviceDetails().id());

            ASSERT_TRUE(LogStatus(site2Stub->StartNode(&ctx, request, &response)).ok());

            auto ks1 = site1.agent->GetKeyStoreFactory()->GetKeyStore(site2.agent->GetConnectionAddress());
            ASSERT_NE(ks1, nullptr);
            KeyID key1Id;
            PSK key1Value;
            ASSERT_TRUE(ks1->GetNewKey(key1Id, key1Value));
            ASSERT_GT(0, key1Value.size());

            auto ks2 = site2.agent->GetKeyStoreFactory()->GetKeyStore(site1.agent->GetConnectionAddress());
            ASSERT_NE(ks1, nullptr);
            PSK key2Value;
            ASSERT_TRUE(ks1->GetExistingKey(key1Id, key2Value).ok());

            ASSERT_EQ(key1Value, key2Value);
        }

        /**
         * @test
         * @brief TEST
         */
        TEST(SiteTest, MultiHop)
        {
            using namespace std;
            using namespace grpc;
            mutex stepMutex;
            condition_variable stepCv;

            DefaultLogger().SetOutputLevel(LogLevel::Trace);
            auto serverCreds = grpc::InsecureServerCredentials();

            MyNetMan netman;
            int netmanPort = 0;
            netman.StartServer(netmanPort, grpc::InsecureServerCredentials());

            ASSERT_NE(netmanPort, 0) << "Failed to start netman";

            // setup default settings
            remote::SiteAgentConfig config1;
            config1.set_name("Site1");
            config1.set_netmanuri("localhost:" + std::to_string(netmanPort));
            // devices for both directions
            config1.mutable_deviceurls()->Add("dummyqkd:///?side=alice&switchname=1234&switchport=1a");
            config1.mutable_deviceurls()->Add("dummyqkd:///?side=bob&switchname=1234&switchport=1b");
            config1.set_listenport(8001);

            // copy the config to the other settings
            remote::SiteAgentConfig config2(config1);
            config2.set_name("Site2");
            config2.set_listenport(8002);

            // copy the config to the other settings
            remote::SiteAgentConfig config3(config1);
            config3.set_name("Site3");
            config3.set_listenport(8003);

            std::vector<remote::Site> registeredSites;

            // expecting a registration
            EXPECT_CALL(netman, OnRegistered(_)).WillRepeatedly(Invoke([&](const remote::Site* site)
            {
                LOGDEBUG("Register called for " + site->url());
                registeredSites.push_back(*site);
                stepCv.notify_one();
                return grpc::Status();
            }));

            // create the site agents
            SiteAgent site1(config1);

            LOGINFO("Site1 = " + site1.GetConnectionAddress());

            // create site one's device
            auto site1dev1 = make_shared<DummyQKD>(remote::Side::Alice, grpc::InsecureChannelCredentials());
            remote::DeviceConfig deviceConfig;
            remote::SessionDetails sessionDetails;
            RemoteQKDDevice adaptor1(site1dev1, serverCreds);

            {
                unique_lock<mutex> lock(stepMutex);
                stepCv.wait(lock);
            }
            ASSERT_EQ(registeredSites.size(), 1) << "Registration 1 failed";

            SiteAgent site2(config2);
            LOGINFO("Site2 = " + site2.GetConnectionAddress());

            {
                unique_lock<mutex> lock(stepMutex);
                stepCv.wait(lock);
            }
            ASSERT_EQ(registeredSites.size(), 2) << "Registration 2 failed";

            {
                LOGINFO("Connecting to " + site2.GetConnectionAddress());
                auto channel = grpc::CreateChannel(site2.GetConnectionAddress(), grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                auto site2Stub = remote::ISiteAgent::NewStub(channel);
                remote::PhysicalPath request;
                google::protobuf::Empty response;

                auto hop = request.mutable_hops()->Add();
                // specify the hop from site 1 alice to site 2 bob
                // To create a curve ball later, start it backwards send 2-1 to 2.
                // When 1 - 2, 2 - 3 is sent later, it should be able to cope
                hop->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config2.deviceurls(1)));
                hop->mutable_first()->set_site(site2.GetConnectionAddress());

                hop->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config1.deviceurls(0)));
                hop->mutable_second()->set_site(site1.GetConnectionAddress());

                LOGTRACE("Calling StartNode 2 - 1 on 2");
                ASSERT_TRUE(LogStatus(site2Stub->StartNode(&ctx, request, &response)).ok());
            }
            LOGTRACE("Register 2 Finished");

            SiteAgent site3(config3);
            LOGINFO("Site3 = " + site3.GetConnectionAddress());

            {
                unique_lock<mutex> lock(stepMutex);
                stepCv.wait(lock);
            }
            ASSERT_EQ(registeredSites.size(), 3) << "Registration 2 failed";

            {
                auto channel = grpc::CreateChannel(site1.GetConnectionAddress(), grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                auto site1Stub = remote::ISiteAgent::NewStub(channel);
                remote::PhysicalPath request;
                google::protobuf::Empty response;

                auto hop1 = request.mutable_hops()->Add();
                // specify the hop from site 1 alice to site 2 bob
                hop1->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config1.deviceurls(0)));
                hop1->mutable_first()->set_site(site1.GetConnectionAddress());

                hop1->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config2.deviceurls(1)));
                hop1->mutable_second()->set_site(site2.GetConnectionAddress());

                auto hop2 = request.mutable_hops()->Add();
                // specify the hop from site 1 alice to site 2 bob
                hop2->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config2.deviceurls(0)));
                hop2->mutable_first()->set_site(site2.GetConnectionAddress());

                hop2->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config3.deviceurls(1)));
                hop2->mutable_second()->set_site(site3.GetConnectionAddress());

                LOGTRACE("Calling StartNode 1 - 2, 2 - 3");
                ASSERT_TRUE(LogStatus(site1Stub->StartNode(&ctx, request, &response)).ok());
            }
            LOGTRACE("Register 3 finished");

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
                ASSERT_TRUE(site1Stub->GetKeyStores(&ctx, request, &response).ok()) << "Failed to get keystores from " + from;
                ASSERT_EQ(response.urls().size(), 2) << "Wrong number of key stores from " + from;
                ASSERT_NE(response.urls(0), response.urls(1)) << "Same keystore twice from " + from;

                // try and get a key from each destination
                for(const auto& ksAddr : response.urls())
                {
                    LOGDEBUG("Getting key from " + from + " to " + ksAddr);
                    grpc::ClientContext keyCtx;
                    remote::KeyRequest keyRequest;
                    remote::SharedKey keyResponse;
                    keyRequest.set_siteto(ksAddr);
                    ASSERT_TRUE(site1Stub->GetSharedKey(&keyCtx, keyRequest, &keyResponse).ok()) << "Failed to get shared key from " + from + " to " + ksAddr;
                    ASSERT_GT(keyResponse.keyid(), 0) << "Invalid key id from " + from + " to " + ksAddr;
                    ASSERT_GT(keyResponse.keyvalue().size(), 0) << "Invalid key size from " + from + " to " + ksAddr;
                    LOGINFO("Success from " + from + " to " + ksAddr);
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
                // specify the hop from site 1 alice to site 2 bob
                hop1->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config1.deviceurls(0)));
                hop1->mutable_first()->set_site(site1.GetConnectionAddress());

                hop1->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config2.deviceurls(1)));
                hop1->mutable_second()->set_site(site2.GetConnectionAddress());

                auto hop2 = request.mutable_hops()->Add();
                // specify the hop from site 1 alice to site 2 bob
                hop2->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config2.deviceurls(1)));
                hop2->mutable_first()->set_site(site2.GetConnectionAddress());

                hop2->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(config3.deviceurls(0)));
                hop2->mutable_second()->set_site(site3.GetConnectionAddress());

                ASSERT_TRUE(LogStatus(site1Stub->EndKeyExchange(&ctx, request, &response)).ok());
            }

        }
    } // namespace tests
} // namespace cqp
