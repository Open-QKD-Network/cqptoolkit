/*!
* @file
* @brief QKDSim
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include <iostream>
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/Application.h"
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/Sites/SiteAgent.h"
#include "CQPToolkit/QKDDevices/DummyQKD.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Util/FileIO.h"
#include "CQPToolkit/QKDDevices/DeviceUtils.h"

#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include <google/protobuf/util/json_util.h>

#include <thread>

using namespace cqp;


/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class QKDSim : public Application
{

    struct Names
    {
        static CONSTSTRING configFile = "config-file";
        static CONSTSTRING id = "id";
        static CONSTSTRING port = "port";
        static CONSTSTRING certFile = "cert";
        static CONSTSTRING keyFile = "key";
        static CONSTSTRING rootCaFile = "rootca";
        static CONSTSTRING tls = "tls";
        static CONSTSTRING connect = "connect";
    };

public:
    /// Constructor
    QKDSim();

    /// Destructor
    ~QKDSim() override = default;

    /// The application's main logic.
    /// Returns an exit code which should be one of the values
    /// from the ExitCode enumeration.
    /// @param[in] args Unprocessed command line arguments
    /// @return success state of the application
    int Main(const std::vector<std::string>& definedArguments) override;

protected:
    /// Called when the option with the given name is encountered
    /// during command line arguments processing.
    ///
    /// The default implementation does option validation, bindings
    /// and callback handling.
    /// @param[in] name Option name
    /// @param[in] value Option Value
    void HandleOption(const CommandArgs::Option& option);

    /// print the help page
    void DisplayHelp(const CommandArgs::Option& option);

    /**
     * @brief HandleVerbose
     * Make the program more verbose
     */
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * Make the program quieter
     */
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

private:
    std::shared_ptr<SiteAgent> siteAgent;

    std::shared_ptr<grpc::ChannelCredentials> clientCreds = grpc::InsecureChannelCredentials();

    /// should the help message be displayed
    bool _helpRequested = false;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };
};

QKDSim::QKDSim()
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    definedArguments.AddOption(Names::configFile, "c", "load configuration data from a file")
    .Bind();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&QKDSim::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::id, "i", "Site Agent ID")
    .Bind();

    definedArguments.AddOption(Names::port, "p", "Listen on this port")
    .Bind();

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&QKDSim::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::connect, "r", "Connect to other site")
    .Bind();

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&QKDSim::HandleVerbose, this, _1));

}

void QKDSim::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;

    definedArguments.PrintHelp(std::cout, "Creates CQP Site Agents for managing QKD systems.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

int QKDSim::Main(const std::vector<std::string> &args)
{
    Application::Main(args);

    if(!stopExecution)
    {
        try
        {
            LOGINFO("Basic application to simulate key exchange");

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
            if(siteSettings.deviceurls().empty())
            {
                siteSettings.mutable_deviceurls()->Add(std::string(DummyQKD::DriverName) + ":///?side=alice");
                siteSettings.mutable_deviceurls()->Add(std::string(DummyQKD::DriverName) + ":///?side=bob");
            }

            siteAgent.reset(new SiteAgent(siteSettings));

            if(definedArguments.IsSet(Names::connect))
            {
                grpc::ClientContext ctx;
                std::shared_ptr<grpc::Channel> siteChannel = grpc::CreateChannel(siteAgent->GetConnectionAddress(), clientCreds);
                std::unique_ptr<remote::ISiteAgent::Stub> siteStub = remote::ISiteAgent::NewStub(siteChannel);

                remote::PhysicalPath request;
                google::protobuf::Empty response;
                remote::HopPair simplePair;

                remote::HopPair* hop1 = request.add_hops();
                hop1->mutable_first()->set_site(siteAgent->GetConnectionAddress());
                hop1->mutable_first()->set_deviceid(DeviceUtils::GetDeviceIdentifier(siteSettings.deviceurls(0)));
                hop1->mutable_second()->set_site(definedArguments.GetStringProp(Names::connect)); // otherSite;
                hop1->mutable_second()->set_deviceid(DeviceUtils::GetDeviceIdentifier(siteSettings.deviceurls(1)));

                grpc::Status result = LogStatus(
                                          siteStub->StartNode(&ctx, request, &response));
            }

            while(!stopExecution)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            LOGDEBUG("Exiting");

        }
        catch(const std::exception& e)
        {
            LOGERROR("Exception: " + std::string(e.what()));
        }


    }

    return 0;
}

CQP_MAIN(QKDSim)
