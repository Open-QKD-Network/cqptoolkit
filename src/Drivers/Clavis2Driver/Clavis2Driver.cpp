/*!
* @file
* @brief Clavis2Driver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Clavis2Driver.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/QKDDevices/ClavisProxy.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"

using namespace cqp;
using namespace std;

struct Clavis2Driver::CommandlineNames
{
    /// Site agent to register with
    static CONSTSTRING siteAgent = "site";
    static CONSTSTRING session = "session";
    static CONSTSTRING configFile = "config";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
    static CONSTSTRING writeConfig = "write-config";
};

Clavis2Driver::Clavis2Driver()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    definedArguments.AddOption(CommandlineNames::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(CommandlineNames::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(CommandlineNames::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&Clavis2Driver::DisplayHelp, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&Clavis2Driver::HandleQuiet, this, _1));

    definedArguments.AddOption(CommandlineNames::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&Clavis2Driver::HandleVerbose, this, _1));

    definedArguments.AddOption(CommandlineNames::siteAgent, "r", "The address of the site agent to register with")
    .Bind();
    definedArguments.AddOption(CommandlineNames::session, "i", "The listen address for the sesion contrl")
    .Bind();

    definedArguments.AddOption(CommandlineNames::configFile, "c", "Filename of the config file to load").HasArgument()
    .Callback(std::bind(&Clavis2Driver::HandleConfigFile, this, _1));
    definedArguments.AddOption(CommandlineNames::writeConfig, "w", "Write the final configuration to the filename.")
    .Bind();

    // set sensible default values for config items
    config.mutable_config()->set_sessionaddress(std::string(net::AnyAddress) + ":0");
    config.set_controladdress(std::string(net::AnyAddress) + ":0");
}

Clavis2Driver::~Clavis2Driver()
{
    adaptor.reset();
    device.reset();
}

void Clavis2Driver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    if(fs::Exists(option.value))
    {
        std::string buffer;
        if(fs::ReadEntireFile(option.value, buffer))
        {
            using namespace google::protobuf::util;
            JsonParseOptions parseOptions;
            if(!LogStatus(JsonStringToMessage(buffer, &config, parseOptions)).ok())
            {
                exitCode = InvalidConfig;
            }
        }
        else
        {
            LOGERROR("Failed to read "  + option.value);
            exitCode = InvalidConfig;
        }
    }
    else
    {
        LOGERROR("File not found: " + option.value);
        exitCode = ConfigNotFound;
    }

    if(exitCode != ExitCodes::Ok)
    {
        stopExecution = true;
        definedArguments.StopOptionsProcessing();
    }
}

void Clavis2Driver::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Simulation of a QKD driver.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}


int Clavis2Driver::Main(const std::vector<std::string>& args)
{
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(CommandlineNames::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(CommandlineNames::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(CommandlineNames::rootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(CommandlineNames::tls))
        {
            creds.set_usetls(true);
        } // if tls set

        definedArguments.GetProp(CommandlineNames::siteAgent, *config.mutable_siteagentaddress());
        definedArguments.GetProp(CommandlineNames::session, *config.mutable_config()->mutable_sessionaddress());

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(CommandlineNames::writeConfig))
        {
            // write out the current settings
            using namespace google::protobuf::util;
            string configJson;
            JsonOptions jsonOptions;
            jsonOptions.add_whitespace = true;
            jsonOptions.preserve_proto_field_names = true;
            if(LogStatus(MessageToJsonString(config, &configJson, jsonOptions)).ok())
            {
                if(!fs::WriteEntireFile(definedArguments.GetStringProp(CommandlineNames::writeConfig), configJson))
                {
                    LOGERROR("Failed to write config to " + definedArguments.GetStringProp(CommandlineNames::writeConfig));
                }
            }
        } // if write config file

        auto channelCreds = LoadChannelCredentials(creds);
        auto serverCreds = LoadServerCredentials(creds);

        device = make_shared<ClavisProxy>(config.config(), channelCreds);
        adaptor = make_unique<RemoteQKDDevice>(device, serverCreds);

        // get the real settings which have been corrected by the device driuver
        (*config.mutable_config()) = device->GetDeviceDetails();
        stopExecution = ! adaptor->StartControlServer(config.controladdress(), config.siteagentaddress());

    } // if !Stop execution

    if(!stopExecution)
    {
        AddSignalHandler(SIGINT, [this](int signum)
        {
            StopProcessing(signum);
        });
        AddSignalHandler(SIGTERM, [this](int signum)
        {
            StopProcessing(signum);
        });
        // Wait for something to stop the driver
        adaptor->WaitForServerShutdown();
    }
    return exitCode;
}

void Clavis2Driver::StopProcessing(int)
{
    // The program is terminating,
    adaptor->StopServer();
}

CQP_MAIN(Clavis2Driver)
