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

#include "SiteAgentRunner.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/FileIO.h"

#include <grpc++/create_channel.h>
#include <grpc++/server_builder.h>
#include <grpc++/client_context.h>
#include <google/protobuf/util/json_util.h>

#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "KeyManagement/Net/ServiceDiscovery.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <thread>
#include "Algorithms/Util/Strings.h"
#include "CQPToolkit/QKDDevices/DeviceUtils.h"

#include <future>

using namespace cqp;

struct Names
{
    static CONSTSTRING configFile = "config-file";
    static CONSTSTRING netman = "netman";
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

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&SiteAgentRunner::HandleVerbose, this, _1));

}

void SiteAgentRunner::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;
    const std::string header = "Creates CQP Site Agents for managing QKD systems.\nCopyright Bristol University. All rights reserved.";

    definedArguments.PrintHelp(std::cout, header);
    definedArguments.StopOptionsProcessing();
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

        if(!definedArguments.GetProp(Names::id, *siteSettings.mutable_id()) && siteSettings.id().empty())
        {
            siteSettings.set_id(UUID());
        }

        uint16_t listenPort = 0;
        if(definedArguments.GetProp(Names::port, listenPort))
        {
            siteSettings.set_listenport(listenPort);
        }

        siteAgents.push_back(new SiteAgent(siteSettings));

        // Connect static links
        for(auto sa : siteAgents)
        {
            sa->ConnectStaticLinks();
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

    while(!stopExecution)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOGDEBUG("Exiting");

    return exitCode;
}

CQP_MAIN(SiteAgentRunner)
