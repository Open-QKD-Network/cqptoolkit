/*!
* @file
* @brief QTunnelServer
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "QTunnelServer.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include <thread>
#include "Algorithms/Util/FileIO.h"
#include <google/protobuf/util/json_util.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/UUID.h"
#include "CQPToolkit/Util/GrpcLogger.h"

using namespace cqp;

struct Names
{
    static CONSTSTRING configFile = "config-file";
    static CONSTSTRING discovery = "nodiscovery";
    static CONSTSTRING startall = "startall";
    static CONSTSTRING controllerId = "id";
    static CONSTSTRING keystoreId = "keystore-id";
    static CONSTSTRING keystoreUrl = "keystore-url";
    static CONSTSTRING port = "port";
    static CONSTSTRING startNode = "start-node";
    static CONSTSTRING endNode = "end-node";
    static CONSTSTRING remote = "remote";
};

QTunnelServer::QTunnelServer()
{
    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    using std::placeholders::_1;
    definedArguments.AddOption(Names::startall, "a", "Start all active tunnels");

    definedArguments.AddOption(Names::configFile, "c", "load configuration data from a file")
    .Bind();

    definedArguments.AddOption(Names::discovery, "z", "Enable ZeroConf discovery");

    definedArguments.AddOption("help", "h", "display help information")
    .Callback(std::bind(&QTunnelServer::HandleHelp, this, _1));

    definedArguments.AddOption(Names::controllerId, "i", "Controller ID").Bind();

    definedArguments.AddOption(Names::keystoreId, "k", "ID for local keystore").Bind();

    definedArguments.AddOption(Names::port, "p", "Listen port").Bind();

    definedArguments.AddOption(Names::startNode, "", "Create a simple tunnel from").Bind();
    definedArguments.AddOption(Names::endNode, "", "Create a simple tunnel to").Bind();
    definedArguments.AddOption(Names::remote, "", "Create a simple with controller").Bind();

    definedArguments.AddOption("quiet", "q", "Decrease output")
    .Callback(std::bind(&QTunnelServer::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::keystoreUrl, "u", "URL for local keystore").Bind();

    definedArguments.AddOption("verbose", "v", "Increase output")
    .Callback(std::bind(&QTunnelServer::HandleVerbose, this, _1));

}

int QTunnelServer::Main(const std::vector<std::string>& args)
{

    Application::Main(args);
    using namespace cqp;
    using namespace std;

    if(!stopExecution)
    {

        if(definedArguments.HasProp(Names::configFile))
        {
            LoadConfig();
        }
        else
        {
            LOGINFO("Loading blank configuration.");
            if(!definedArguments.GetProp(Names::controllerId, *controllerSettings.mutable_id()))
            {
                controllerSettings.set_id(UUID());
            }

            if(!definedArguments.GetProp(Names::keystoreId, *controllerSettings.mutable_localkeyfactoryuuid()))
            {
                definedArguments.GetProp(Names::keystoreUrl, *controllerSettings.mutable_localkeyfactoryuri());
            }

            if(definedArguments.GetProp(Names::port, listenPort))
            {
                controllerSettings.set_listenport(listenPort);
            }
            controller.reset(new tunnels::Controller(controllerSettings));
        }

        if(controller)
        {

            // create the server
            grpc::ServerBuilder builder;
            // grpc will create worker threads as it needs, idle work threads
            // will be stopped if there are more than this number running
            // setting this too low causes large number of thread creation+deletions, default = 2
            builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 50);

            builder.AddListeningPort(std::string(net::AnyAddress) + ":" + std::to_string(listenPort), LoadServerCredentials(controllerSettings.credentials()), &listenPort);

            LOGTRACE("Registering services");
            // Register services
            builder.RegisterService(static_cast<remote::tunnels::ITunnelServer::Service*>(controller.get()));
            builder.RegisterService(&reportServer);
            // ^^^ Add new services here ^^^ //

            LOGTRACE("Starting server");
            server = builder.BuildAndStart();
            if(!server)
            {
                LOGERROR("Failed to create server");
                exitCode = ExitCodes::ServiceCreationFailed;
            }
            else
            {
                LOGINFO("My address is: " + net::GetHostname() + ":" + std::to_string(listenPort));
                if(definedArguments.IsSet(Names::discovery) || controllerSettings.useautodiscover())
                {
                    LOGTRACE("Start service discovery");
                    sd.reset(new net::ServiceDiscovery());
                    RemoteHost service;
                    remote::tunnels::ControllerDetails settings;
                    controller->GetControllerSettings(settings);

                    if(settings.name().empty())
                    {
                        service.name = "QTunnelServer-" + std::to_string(listenPort);
                    }
                    else
                    {
                        service.name = settings.name();
                    }
                    service.id = settings.id();
                    service.interfaces.insert(remote::tunnels::ITunnelServer::service_full_name());
                    service.interfaces.insert(remote::IReporting::service_full_name());
                    // ^^^ Add new services here ^^^ //
                    service.port = listenPort;

                    sd->SetServices(service);
                    sd->Add(controller.get());
                }

                if(definedArguments.IsSet(Names::startall))
                {
                    controller->StartAllTunnels();
                }

                if(definedArguments.IsSet(Names::startNode) && definedArguments.IsSet(Names::endNode) &&
                        definedArguments.IsSet(Names::remote))
                {
                    remote::tunnels::Tunnel tun;

                    tun.set_name("simple-tunnel");
                    tun.mutable_startnode()->set_clientdataporturi(definedArguments.GetStringProp(Names::startNode));
                    tun.mutable_endnode()->set_clientdataporturi(definedArguments.GetStringProp(Names::endNode));
                    tun.set_remotecontrolleruri(definedArguments.GetStringProp(Names::remote));

                    tun.mutable_keylifespan()->set_maxbytes(1024*64);
                    tun.mutable_keylifespan()->mutable_maxage()->set_seconds(10);
                    LOGINFO("Setting key lifespan to " + std::to_string(tun.keylifespan().maxbytes()) + " bytes"
                            " or " + std::to_string(tun.keylifespan().maxage().seconds()) + " seconds");

                    controller->ModifyTunnel(tun);
                    LOGINFO("Starting tunnel " + tun.name());
                    LogStatus(controller->StartTunnel(tun.name()));

                }
                //TODO shutdown gracefully
                server->Wait();
            }
        }
    }
    return exitCode;
}

bool QTunnelServer::LoadConfig()
{
    bool result = false;
    try
    {
        std::string configFilename;

        LOGTRACE("Loading config");
        if(definedArguments.GetProp(Names::configFile, configFilename) && fs::Exists(configFilename))
        {
            using namespace google::protobuf;

            std::string configData;
            fs::ReadEntireFile(configFilename, configData);
            util::Status loadStatus = util::JsonStringToMessage(configData, &controllerSettings);

            if(LogStatus(loadStatus).ok())
            {
                LOGINFO("Loading configuration for " + controllerSettings.name());
                controller.reset(new tunnels::Controller(controllerSettings));
                listenPort = controllerSettings.listenport();
                result = true;
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
    catch (const std::exception& e)
    {
        LOGERROR(e.what());
        exitCode = ExitCodes::UnknownError;
    }

    return result;
}

void QTunnelServer::HandleHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Creates encrypted tunnels using QKD keys.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

CQP_MAIN(QTunnelServer)
