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
                remote::DeviceConfig config;
                config.set_side(side);
                device = make_shared<DummyQKD>(config, grpc::InsecureChannelCredentials());
                LOGTRACE("Creating adaptor");
                adaptor = make_shared<RemoteQKDDevice>(device, serverCreds);
                if(adaptor->StartControlServer("localhost:0", siteAgentAddress))
                {
                    controlAddr = adaptor->GetControlAddress();
                    LOGINFO("Remote device control available on port " + std::to_string(controlAddr.GetPort()));
                }
                else
                {
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
            URI controlAddr;
            grpc::Status result;
        };

        struct SiteAgentBuilder
        {
            SiteAgentBuilder(const std::string& name, uint16_t port)
            {
                using namespace std;
                config.set_name(name);
                config.set_listenport(port);
                config.set_fallbackkey("abcdefgijklmnopq");
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
            auto site2Stub = remote::ISiteAgent::NewStub(channel);

            remote::PhysicalPath request;

            auto hop = request.add_hops();
            hop->mutable_first()->set_site(site2.agent->GetConnectionAddress());
            hop->mutable_first()->set_deviceid(site2bob.device->GetDeviceDetails().id());
            hop->mutable_second()->set_site(site1.agent->GetConnectionAddress());
            hop->mutable_second()->set_deviceid(site1alice.device->GetDeviceDetails().id());

            {
                grpc::ClientContext ctx;
                google::protobuf::Empty response;

                ASSERT_TRUE(LogStatus(site2Stub->StartNode(&ctx, request, &response)).ok());
            }

            auto ks1 = site1.agent->GetKeyStoreFactory()->GetKeyStore(site2.agent->GetConnectionAddress());
            ASSERT_NE(ks1, nullptr);
            KeyID key1Id;
            PSK key1Value;
            ASSERT_TRUE(ks1->GetNewKey(key1Id, key1Value, true));
            ASSERT_GT(key1Value.size(), 0);

            auto ks2 = site2.agent->GetKeyStoreFactory()->GetKeyStore(site1.agent->GetConnectionAddress());
            ASSERT_NE(ks2, nullptr);
            PSK key2Value;
            ASSERT_TRUE(ks2->GetExistingKey(key1Id, key2Value).ok());

            ASSERT_EQ(key1Value, key2Value);

            {
                grpc::ClientContext ctx;
                google::protobuf::Empty response;
                // bring the system down
                ASSERT_TRUE(LogStatus(site2Stub->EndKeyExchange(&ctx, request, &response)).ok());
            }

            // this should cause the device to be unregistered
            site1alice.adaptor.reset();
            site2bob.adaptor.reset();
        }

        /**
         * @test
         * @brief TEST
         */
        TEST(SiteTest, MultiHop)
        {
            using namespace std;

            SiteAgentBuilder site1("Site1", 8001);
            // create site 1's device
            SiteTestCollection site1alice(remote::Side::Alice, site1.agent->GetConnectionAddress());
            ASSERT_TRUE(site1alice.result.ok());

            SiteAgentBuilder site2("Site2", 8002);
            SiteTestCollection site2alice(remote::Side::Alice, site2.agent->GetConnectionAddress());
            SiteTestCollection site2bob(remote::Side::Bob, site2.agent->GetConnectionAddress());
            ASSERT_TRUE(site2alice.result.ok());
            ASSERT_TRUE(site2bob.result.ok());

            SiteAgentBuilder site3("Site3", 8003);
            SiteTestCollection site3bob(remote::Side::Bob, site3.agent->GetConnectionAddress());
            ASSERT_TRUE(site3bob.result.ok());

            // setup complete

            LOGINFO("Connecting to " + site1.agent->GetConnectionAddress());
            auto site1Channel = grpc::CreateChannel(site1.agent->GetConnectionAddress(), grpc::InsecureChannelCredentials());
            auto site1Stub = remote::ISiteAgent::NewStub(site1Channel);

            remote::PhysicalPath request;

            auto hop1 = request.add_hops();
            hop1->mutable_first()->set_site(site1.agent->GetConnectionAddress());
            hop1->mutable_first()->set_deviceid(site1alice.device->GetDeviceDetails().id());
            hop1->mutable_second()->set_site(site2.agent->GetConnectionAddress());
            hop1->mutable_second()->set_deviceid(site2bob.device->GetDeviceDetails().id());

            auto hop2 = request.add_hops();
            hop2->mutable_first()->set_site(site2.agent->GetConnectionAddress());
            hop2->mutable_first()->set_deviceid(site2alice.device->GetDeviceDetails().id());
            hop2->mutable_second()->set_site(site3.agent->GetConnectionAddress());
            hop2->mutable_second()->set_deviceid(site3bob.device->GetDeviceDetails().id());

            {
                grpc::ClientContext ctx;
                google::protobuf::Empty response;

                ASSERT_TRUE(LogStatus(site1Stub->StartNode(&ctx, request, &response)).ok());
            }

            auto ks1 = site1.agent->GetKeyStoreFactory()->GetKeyStore(site3.agent->GetConnectionAddress());
            ASSERT_NE(ks1, nullptr);
            KeyID key1Id;
            PSK key1Value;
            ASSERT_TRUE(ks1->GetNewKey(key1Id, key1Value, true));
            ASSERT_GT(key1Value.size(), 0);

            auto ks3 = site3.agent->GetKeyStoreFactory()->GetKeyStore(site1.agent->GetConnectionAddress());
            ASSERT_NE(ks3, nullptr);
            PSK key3Value;
            ASSERT_TRUE(ks3->GetExistingKey(key1Id, key3Value).ok());

            ASSERT_EQ(key1Value, key3Value);

            {
                grpc::ClientContext ctx;
                google::protobuf::Empty response;
                // bring the system down
                ASSERT_TRUE(LogStatus(site1Stub->EndKeyExchange(&ctx, request, &response)).ok());
            }

            // this should cause the device to be unregistered
            site1alice.adaptor.reset();
            site2alice.adaptor.reset();
            site2bob.adaptor.reset();
            site3bob.adaptor.reset();

        }
    } // namespace tests
} // namespace cqp
