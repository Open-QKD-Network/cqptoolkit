/*!
* @file
* @brief DriverApplication
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DriverApplication.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include <iostream>
#include "QKDInterfaces/Device.pb.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Util/FileIO.h"

namespace cqp
{

    DriverApplication::DriverApplication()
    {
        using std::placeholders::_1;

        cqp::ConsoleLogger::Enable();

        definedArguments.AddOption(CommandlineNames::certFile, "", "Certificate file")
        .Bind();
        definedArguments.AddOption(CommandlineNames::certKeyFile, "", "Certificate key file")
        .Bind();
        definedArguments.AddOption(CommandlineNames::rootCaFile, "", "Certificate authority file")
        .Bind();

        definedArguments.AddOption("help", "h", "display help information on command line arguments")
        .Callback(std::bind(&DriverApplication::DisplayHelp, this, _1));

        definedArguments.AddOption("", "q", "Decrease output")
        .Callback(std::bind(&DriverApplication::HandleQuiet, this, _1));

        definedArguments.AddOption(CommandlineNames::tls, "s", "Use secure connections");

        definedArguments.AddOption("", "v", "Increase output")
        .Callback(std::bind(&DriverApplication::HandleVerbose, this, _1));

        definedArguments.AddOption(CommandlineNames::controlAddr, "k", "Listen address (host and port) for control interface")
        .Bind();

        definedArguments.AddOption(CommandlineNames::siteAgent, "r", "The address of the site agent to register with")
        .Bind();

        definedArguments.AddOption(CommandlineNames::configFile, "c", "Filename of the config file to load").HasArgument()
        .Callback(std::bind(&DriverApplication::HandleConfigFile, this, _1));

        definedArguments.AddOption(CommandlineNames::switchName, "", "The OpenFlow ID of the switch the device is connected to")
        .Bind();
        definedArguments.AddOption(CommandlineNames::switchPort, "", "The OpenFlow ID of the port on the switch")
        .Bind();

        // set sensible default values for config items
        controlDetails->set_controladdress(std::string(net::AnyAddress) + ":0");
    }

    void DriverApplication::HandleVerbose(const CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    void DriverApplication::HandleQuiet(const CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

    bool DriverApplication::ParseConfigFile(const CommandArgs::Option& option, google::protobuf::Message& config)
    {
        bool result = false;
        if(fs::Exists(option.value))
        {
            std::string buffer;
            if(fs::ReadEntireFile(option.value, buffer))
            {
                using namespace google::protobuf::util;
                JsonParseOptions parseOptions;
                result = LogStatus(JsonStringToMessage(buffer, &config, parseOptions)).ok();
            }
            else
            {
                LOGERROR("Failed to read "  + option.value);
            }
        }
        else
        {
            LOGERROR("File not found: " + option.value);
        }

        if(!result)
        {
            stopExecution = true;
            definedArguments.StopOptionsProcessing();
        }

        return result;
    }

    bool DriverApplication::WriteConfigFile(const google::protobuf::Message& config, const std::string& filename)
    {
        bool result = false;
        // write out the current settings
        using namespace google::protobuf::util;
        std::string configJson;
        JsonOptions jsonOptions;
        jsonOptions.add_whitespace = true;
        jsonOptions.preserve_proto_field_names = true;
        jsonOptions.always_print_primitive_fields = true;
        if(LogStatus(MessageToJsonString(config, &configJson, jsonOptions)).ok())
        {
            result = fs::WriteEntireFile(filename, configJson);
            if(!result)
            {
                LOGERROR("Failed to write config to " + filename);
            }
        }
        return result;
    }

    int DriverApplication::Main(const std::vector<std::string>& args)
    {

        using namespace cqp;
        using namespace std;
        exitCode = Application::Main(args);

        if(!stopExecution)
        {
            definedArguments.GetProp(CommandlineNames::certFile, *creds.mutable_certchainfile());
            definedArguments.GetProp(CommandlineNames::certKeyFile, *creds.mutable_privatekeyfile());
            definedArguments.GetProp(CommandlineNames::rootCaFile, *creds.mutable_rootcertsfile());

            if(definedArguments.IsSet(CommandlineNames::tls))
            {
                creds.set_usetls(true);
            } // if tls set

            definedArguments.GetProp(CommandlineNames::siteAgent, *controlDetails->mutable_siteagentaddress());
            definedArguments.GetProp(CommandlineNames::controlAddr, *controlDetails->mutable_controladdress());

            definedArguments.GetProp(CommandlineNames::switchName, *controlDetails->mutable_config()->mutable_switchname());
            definedArguments.GetProp(CommandlineNames::switchPort, *controlDetails->mutable_config()->mutable_switchport());

            channelCreds = LoadChannelCredentials(creds);
            serverCreds = LoadServerCredentials(creds);
        }

        return exitCode;
    }

    void DriverApplication::ShutdownNow()
    {
        adaptor->StopServer();
        adaptor.reset();

        Application::ShutdownNow();
    }

    void DriverApplication::DisplayHelp(const CommandArgs::Option&)
    {
        definedArguments.PrintHelp(std::cout, "Driver for QKD unit using session control and proves key through the IKey interface.\nCopyright Bristol University. All rights reserved.");
        definedArguments.StopOptionsProcessing();
        stopExecution = true;
    }

} // namespace cqp
