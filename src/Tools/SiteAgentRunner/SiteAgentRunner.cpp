/*!
* @file
* @brief SiteAgentRunner
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Util/ConsoleLogger.h"
#include "CQPToolkit/Drivers/DeviceFactory.h"
#include "CQPToolkit/Util/FileIO.h"

#include <grpc++/create_channel.h>
#include <grpc++/server_builder.h>
#include <grpc++/client_context.h>
#include <google/protobuf/util/json_util.h>

#include "CQPToolkit/KeyGen/KeyStoreFactory.h"
#include "CQPToolkit/KeyGen/KeyStore.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "CQPToolkit/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <thread>
#include "CQPToolkit/Util/Util.h"
#include "QKDInterfaces/ISiteDetails.grpc.pb.h"

// Site agent compatible drivers
#include "CQPToolkit/Drivers/DummyQKD.h"
#include "CQPToolkit/Drivers/ClavisProxy.h"
// ^^^^^^ Add new drivers here ^^^^^^^^^

#include "SiteAgentRunner.h"
#include <future>

using namespace cqp;

struct Names
{
    static CONSTSTRING configFile = "config-file";
    static CONSTSTRING netman = "netman";
    static CONSTSTRING test = "test";
    static CONSTSTRING id = "id";
    static CONSTSTRING discovery = "discovery";
    static CONSTSTRING port = "port";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
    static CONSTSTRING bs = "backingstore";
    static CONSTSTRING bsurl = "bsurl";
    struct BackingStores
    {
        static NAMEDSTRING(none);
        static NAMEDSTRING(file);
        static NAMEDSTRING(pkcs11);
    };
};

SiteAgentRunner::SiteAgentRunner()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    GrpcAllowMACOnlyCiphers();

    definedArguments.AddOption(Names::netman, "a", "Address of the Quantum Path Agent")
    .Bind();

    definedArguments.AddOption(Names::configFile, "c", "load configuration data from a file")
    .Bind();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("device", "d", "Device to use")
    .HasArgument()
    .Callback(std::bind(&SiteAgentRunner::HandleDevice, this, _1));

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&SiteAgentRunner::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::id, "i", "Site Agent ID")
    .Bind();

    definedArguments.AddOption(Names::bs, "b", "Backing Store (none, file)")
    .Bind();
    definedArguments.AddOption(Names::bsurl, "u", "Backing Store URL")
    .Bind();


    definedArguments.AddOption(Names::discovery, "z", "Enable ZeroConf discovery");

    definedArguments.AddOption(Names::port, "p", "Listen on this port")
    .Bind();

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&SiteAgentRunner::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");
    definedArguments.AddOption(Names::test, "t", "Testing mode, start multiple dummy agents");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&SiteAgentRunner::HandleVerbose, this, _1));

    // Site agent drivers
    DummyQKD::RegisterWithFactory();
    ClavisProxy::RegisterWithFactory();
    // ^^^^^^ Add new drivers here ^^^^^^^^^

}

void SiteAgentRunner::HandleDevice(const CommandArgs::Option& option)
{
    deviceAddresses.push_back(option.value);
}

void SiteAgentRunner::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;
    const vector<string> drivers = DeviceFactory::GetKnownDrivers();
    string driverNames;
    for(auto driver : drivers)
    {
        if(!driverNames.empty())
        {
            driverNames += ", ";
        }
        driverNames += driver;
    }

    const std::string header = "Creates CQP Site Agents for managing QKD systems.\nCopyright Bristol University. All rights reserved.";
    const std::string footer = "Drivers: " + driverNames + "\n" +
            "Device urls: [driver]://[location[:port]]/[?parameters]]\n" +
            "   The type of location and parameters are driver dependant.\n" +
            "Common parameters:\n" +
            "   Parameter           Values              Description\n" +
            "   side                alice|bob|any       Which kind of device\n" +
            "   switchName          <openflow string>   Connected optical switch\n" +
            "   switchPort          <port id>           Switch connector id\n" +
            "   keybytes            16|32               Bytes per key generated\n";

    definedArguments.PrintHelp(std::cout, header, footer);
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void SiteAgentRunner::RunTests(remote::SiteAgentConfig& siteSettings)
{
    using namespace cqp;
    using namespace std;

    LOGINFO("Testing SiteAgents Starting...");
    const std::string device1 = std::string(DummyQKD::DriverName) + ":///?side=alice&port=dummy1";
    const std::string device2a = std::string(DummyQKD::DriverName) + ":///?side=bob&port=dummy2a";
    const std::string device2b = std::string(DummyQKD::DriverName) + ":///?side=alice&port=dummy2b";
    const std::string device3 = std::string(DummyQKD::DriverName) + ":///?side=bob&port=dummy3";
    remote::SiteAgentConfig siteAConfig = siteSettings;
    remote::SiteAgentConfig siteBConfig = siteAConfig;
    remote::SiteAgentConfig siteCConfig = siteAConfig;

    siteAConfig.add_deviceurls(device1);
    siteAConfig.set_id("1360cb73-7384-4a0a-a7ab-1018e1310a88");
    siteAConfig.set_name("SiteA");

    siteBConfig.add_deviceurls(device2a);
    siteBConfig.add_deviceurls(device2b);
    siteBConfig.set_id("6f319c73-7bb5-4cd1-b87e-f31e21d58884");
    siteBConfig.set_name("SiteB");

    siteCConfig.add_deviceurls(device3);
    siteCConfig.set_id("47b480f5-e9d8-43a6-be4e-67a3ad6eb031");
    siteCConfig.set_name("SiteC");

    siteAgents.push_back(new SiteAgent(siteAConfig));
    siteAgents.push_back(new SiteAgent(siteBConfig));
    siteAgents.push_back(new SiteAgent(siteCConfig));

    {
        // dump the config
        std::string temp;
        google::protobuf::util::JsonOptions jsonOpts;
        jsonOpts.add_whitespace = true;
        jsonOpts.preserve_proto_field_names = true;

        google::protobuf::util::MessageToJsonString(siteAConfig, &temp, jsonOpts);
        fs::WriteEntireFile("DummySiteA.json", temp);
        temp.clear();
        google::protobuf::util::MessageToJsonString(siteBConfig, &temp, jsonOpts);
        fs::WriteEntireFile("DummySiteB.json", temp);
        temp.clear();
        google::protobuf::util::MessageToJsonString(siteCConfig, &temp, jsonOpts);
        fs::WriteEntireFile("DummySiteC.json", temp);
    }

    sd.reset(new net::ServiceDiscovery());
    {
        int count = 1;

        for(auto sa : siteAgents)
        {
            LOGDEBUG("Site agent " + std::to_string(count) + ": " + sa->GetConnectionAddress());
            count++;
            sa->RegisterWithDiscovery(*sd);
        }
    }
    std::shared_ptr<grpc::Channel> siteChannel = grpc::CreateChannel(siteAgents[0]->GetConnectionAddress(), LoadChannelCredentials(siteSettings.credentials()));
    std::unique_ptr<remote::ISiteAgent::Stub> siteStub = remote::ISiteAgent::NewStub(siteChannel);

    auto fut = std::async(std::launch::async, [&]()
    {
        std::unique_ptr<remote::ISiteDetails::Stub> detailsStub = remote::ISiteDetails::NewStub(siteChannel);
        grpc::ClientContext ctx;
        google::protobuf::Empty request;
        auto reader = detailsStub->GetLinkStatus(&ctx, request);
        remote::LinkStatus message;

        while(reader && reader->Read(&message))
        {
            std::string stateString;
            switch (message.state())
            {
            case remote::LinkStatus_State_Inactive:
                stateString = "Inactive";
                break;
            case remote::LinkStatus_State_Connecting:
                stateString = "Connecting";
                break;
            case remote::LinkStatus_State_ConnectionEstablished:
                stateString = "ConnectionEstablished";
                break;
            case remote::LinkStatus_State_LinkStatus_State_INT_MAX_SENTINEL_DO_NOT_USE_:
            case remote::LinkStatus_State_LinkStatus_State_INT_MIN_SENTINEL_DO_NOT_USE_:
                break;
            }
            LOGINFO("State changed to " + stateString + " for " + message.siteto() + ", code=" + std::to_string(message.errorcode()));
        }
    });
    //for(auto otherSite : otherSites)
    {
        grpc::ClientContext ctx;
        remote::PhysicalPath request;
        google::protobuf::Empty response;
        remote::HopPair simplePair;

        remote::HopPair* hop1 = request.add_hops();
        hop1->mutable_first()->set_site(siteAgents[0]->GetConnectionAddress());
        hop1->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device1));
        hop1->mutable_second()->set_site(siteAgents[1]->GetConnectionAddress()); // otherSite;
        hop1->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device2a));

        LOGINFO("Starting direct key exchange from " + siteAgents[0]->GetConnectionAddress() + " to " + siteAgents[1]->GetConnectionAddress());
        grpc::Status result = LogStatus(
                                  siteStub->StartNode(&ctx, request, &response));

    }

    // create a 2 hop link
    {
        grpc::ClientContext ctx;
        remote::PhysicalPath request;
        google::protobuf::Empty response;
        remote::HopPair simplePair;

        remote::HopPair* hop1 = request.add_hops();
        hop1->mutable_first()->set_site(siteAgents[0]->GetConnectionAddress());
        hop1->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device1));
        hop1->mutable_second()->set_site(siteAgents[1]->GetConnectionAddress()); // otherSite;
        hop1->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device2a));

        remote::HopPair* hop2 = request.add_hops();
        hop2->mutable_first()->set_site(siteAgents[1]->GetConnectionAddress());
        hop2->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device2b));
        hop2->mutable_second()->set_site(siteAgents[2]->GetConnectionAddress()); // otherSite;
        hop2->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device3));

        LOGINFO("Starting indirect key exchange from " + siteAgents[0]->GetConnectionAddress() + " to " + siteAgents[2]->GetConnectionAddress());
        grpc::Status result = LogStatus(
                                  siteStub->StartNode(&ctx, request, &response));

    }

    shared_ptr<keygen::KeyStore> ksab_a = siteAgents[0]->GetKeyStoreFactory()->GetKeyStore(
            siteAgents[1]->GetConnectionAddress());
    shared_ptr<keygen::KeyStore> ksab_b = siteAgents[1]->GetKeyStoreFactory()->GetKeyStore(
            siteAgents[0]->GetConnectionAddress());

    const std::vector<std::string> path_ac =
    {
        siteAgents[0]->GetConnectionAddress(),
        siteAgents[1]->GetConnectionAddress(),
        siteAgents[2]->GetConnectionAddress()
    };

    shared_ptr<keygen::KeyStore> ksac_a = siteAgents[0]->GetKeyStoreFactory()->GetKeyStore(
            siteAgents[2]->GetConnectionAddress());
    // add the middle hop
    ksac_a->SetPath({siteAgents[1]->GetConnectionAddress()});
    shared_ptr<keygen::KeyStore> ksac_c = siteAgents[2]->GetKeyStoreFactory()->GetKeyStore(
            siteAgents[0]->GetConnectionAddress());
    // add the middle hop
    ksac_c->SetPath({siteAgents[1]->GetConnectionAddress()});

    bool keepGoing = true;
    int keysRecieved = 0;
    while(keepGoing && keysRecieved < 10)
    {
        this_thread::sleep_for(chrono::milliseconds(2000));
        if(ksab_a->GetNumberUnusedKeys() > 100)
        {

            grpc::ClientContext ctx;
            remote::PhysicalPath request;
            google::protobuf::Empty response;
            remote::HopPair simplePair;

            remote::HopPair* hop1 = request.add_hops();
            hop1->mutable_first()->set_site(siteAgents[0]->GetConnectionAddress());
            hop1->mutable_first()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device1));
            hop1->mutable_second()->set_site(siteAgents[1]->GetConnectionAddress()); // otherSite;
            hop1->mutable_second()->set_deviceid(DeviceFactory::GetDeviceIdentifier(device2a));

            LOGINFO("Stopping key exchange from " + siteAgents[0]->GetConnectionAddress() + " to " + siteAgents[1]->GetConnectionAddress());
            LogStatus(siteStub->EndKeyExchange(&ctx, request, &response));

            grpc::ClientContext ctx2;
            response.Clear();
            LOGINFO("Starting key exchange from " + siteAgents[0]->GetConnectionAddress() + " to " + siteAgents[1]->GetConnectionAddress());
            LogStatus(siteStub->StartNode(&ctx2, request, &response));
        }
        KeyID id;
        PSK key1Value;
        PSK key2Value;
        if(ksab_a->GetNewKey(id, key1Value) && LogStatus(ksab_b->GetExistingKey(id, key2Value)).ok())
        {
            if(key1Value != key2Value)
            {
                LOGERROR("Direct Keys dont match");
                keepGoing = false;
                exitCode = 1;
            }
            else
            {
                LOGINFO("Direct Keys match!");
                keysRecieved++;
            }
        }
        else
        {
            LOGERROR("Failed to get Direct keys");
        }

        key1Value.clear();
        key2Value.clear();

        if(ksac_a->GetNewKey(id, key1Value) && LogStatus(ksac_c->GetExistingKey(id, key2Value)).ok())
        {
            if(key1Value != key2Value)
            {
                LOGERROR("Indirect Keys dont match");
                keepGoing = false;
                exitCode = 2;
            }
            else
            {
                LOGINFO("Indirect Keys match!");
                keysRecieved++;
            }
        }
        else
        {
            LOGERROR("Failed to get Indirect keys");
        }
    } // while keep going

    for(auto sa : siteAgents)
    {
        delete sa;
    }
    siteAgents.clear();
    stopExecution = true;
}

int SiteAgentRunner::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        remote::SiteAgentConfig siteSettings;
        std::string configFilename;
        if(definedArguments.GetProp(Names::configFile, configFilename))
        {
            if(fs::Exists(configFilename))
            {
                using namespace google::protobuf;

                std::string configData;
                fs::ReadEntireFile(configFilename, configData);
                util::Status loadStatus = util::JsonStringToMessage(configData, &siteSettings);

                if(LogStatus(loadStatus).ok())
                {
                    LOGINFO("Loading configuration for " + siteSettings.name());
                }
                else
                {
                    LOGERROR("Invalid configuration");
                    exitCode = ExitCodes::InvalidConfig;
                }
            }
            else
            {
                LOGERROR("File not found: " + configFilename);
                exitCode = ExitCodes::ConfigNotFound;
            }
        }

        siteSettings.mutable_credentials()->set_usetls(definedArguments.IsSet(Names::tls));
        definedArguments.GetProp(Names::certFile, *siteSettings.mutable_credentials()->mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *siteSettings.mutable_credentials()->mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *siteSettings.mutable_credentials()->mutable_rootcertsfile());

        definedArguments.GetProp(Names::netman, *siteSettings.mutable_netmanuri());
        definedArguments.GetProp(Names::bsurl, *siteSettings.mutable_backingstoreurl());

        if(definedArguments.IsSet(Names::test))
        {
            RunTests(siteSettings);
        }
        else
        {
            if(!definedArguments.GetProp(Names::id, *siteSettings.mutable_id()) && siteSettings.id().empty())
            {
                siteSettings.set_id(UUID());
            }

            uint16_t listenPort = 0;
            if(definedArguments.GetProp(Names::port, listenPort))
            {
                siteSettings.set_listenport(listenPort);
            }

            // configure devices
            for(auto devName : deviceAddresses)
            {
                LOGTRACE("Adding device: " + devName);
                siteSettings.add_deviceurls(devName);
            }
            siteAgents.push_back(new SiteAgent(siteSettings));

            // Connect static links
            for(auto sa : siteAgents)
            {
                sa->ConnectStaticLinks();
            }

            // Register for dynamic links
            for(auto sa : siteAgents)
            {
                sa->RegisterWithNetMan();
            }

            if(definedArguments.IsSet(Names::discovery) || siteSettings.useautodiscover())
            {
                sd.reset(new net::ServiceDiscovery());
                for(auto sa : siteAgents)
                {
                    sa->RegisterWithDiscovery(*sd);
                }
            }
        }

    }

    while(!stopExecution)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOGDEBUG("Exiting");

    return exitCode;
}

CQP_MAIN(SiteAgentRunner)
