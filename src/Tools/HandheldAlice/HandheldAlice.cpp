/*!
* @file
* @brief StatsDump
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "HandheldAlice.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include <iostream>
#include "Algorithms/Util/Strings.h"
#include <grpc++/server_builder.h>
#include "CQPToolkit/Auth/AuthUtil.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "CQPToolkit/Session/AliceSessionController.h"
#include "Algorithms/Util/FileIO.h"

using namespace cqp;

struct Names
{
    static CONSTSTRING connect = "connect";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
    static CONSTSTRING keyServer = "key-serv";
    static CONSTSTRING sessionAddr = "session-addr";
    static CONSTSTRING sessionPort = "session-port";
    static CONSTSTRING device = "device";
    static CONSTSTRING usbDevice = "usb-device";
    static CONSTSTRING configFile = "config";
    static CONSTSTRING writeConfig = "write-config";
};

HandheldAlice::HandheldAlice()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&HandheldAlice::DisplayHelp, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&HandheldAlice::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&HandheldAlice::HandleVerbose, this, _1));


    definedArguments.AddOption(Names::keyServer, "k", "Listen address for serving keys")
    .Bind();
    definedArguments.AddOption(Names::sessionAddr, "", "Bind address for internal communication")
    .Bind();
    definedArguments.AddOption(Names::sessionPort, "", "Port for internal communication")
    .Bind();
    definedArguments.AddOption(Names::device, "d", "The serial device to use, otherwise the first serial device will be used")
    .Bind();
    definedArguments.AddOption(Names::usbDevice, "u", "The serial number for the usb device to use, otherwise use the first detected")
    .Bind();
    definedArguments.AddOption(Names::connect, "b", "The address of Bob's session controller. Required")
    .Bind().Required();

    definedArguments.AddOption(Names::configFile, "c", "Filename of the config file to load").HasArgument()
    .Callback(std::bind(&HandheldAlice::HandleConfigFile, this, _1));
    definedArguments.AddOption(Names::writeConfig, "w", "Write the final configuration to the filename .")
    .Bind();

    // set sensible default values for config items
    config.set_sessionport(0);
    config.set_sessionaddress("0.0.0.0");
    config.set_keylistenaddress("localhost:0");
    config.mutable_params()->set_photonsperburst(LEDAliceMk1::DefaultParameters.photonsPerBurst);
    config.mutable_params()->set_markerfraction(LEDAliceMk1::DefaultParameters.markerFraction);
}

void HandheldAlice::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Outputs statistics from CQP services in CSV format.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void HandheldAlice::HandleConfigFile(const CommandArgs::Option& option)
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
            else if(!config.bobaddress().empty())
            {
                // dont need this option if the config file specifies it
                definedArguments[Names::connect]->set = true;
            }
        } else {
            LOGERROR("Failed to read "  + option.value);
            exitCode = InvalidConfig;
        }
    } else {
        LOGERROR("File not found: " + option.value);
        exitCode = ConfigNotFound;
    }

    if(exitCode != ExitCodes::Ok)
    {
        stopExecution = true;
        definedArguments.StopOptionsProcessing();
    }
}

int HandheldAlice::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(Names::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(Names::tls))
        {
            creds.set_usetls(true);
        } // if tls set

        definedArguments.GetProp(Names::connect, *config.mutable_bobaddress());
        definedArguments.GetProp(Names::keyServer, *config.mutable_keylistenaddress());
        definedArguments.GetProp(Names::sessionAddr, *config.mutable_sessionaddress());
        definedArguments.GetProp(Names::device, *config.mutable_devicename());
        definedArguments.GetProp(Names::usbDevice, *config.mutable_usbdevicename());

        {/* temp scope */
            uint32_t temp;
            if(definedArguments.GetProp(Names::sessionPort, temp))
            {
                config.set_sessionport(temp);
            }
        }/* temp scope */

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(Names::writeConfig))
        {
            // write out the current settings
            using namespace google::protobuf::util;
            string configJson;
            JsonOptions jsonOptions;
            jsonOptions.add_whitespace = true;
            jsonOptions.preserve_proto_field_names = true;
            if(LogStatus(MessageToJsonString(config, &configJson, jsonOptions)).ok())
            {
                if(!fs::WriteEntireFile(definedArguments.GetStringProp(Names::writeConfig), configJson))
                {
                    LOGERROR("Failed to write config to " + definedArguments.GetStringProp(Names::writeConfig));
                }
            }
        } // if write config file

        driver = make_shared<LEDAliceMk1>(LoadChannelCredentials(creds), config.devicename(), config.usbdevicename());

        if(driver)
        {
            driver->SetParameters({
                                      config.params().photonsperburst(),
                                      config.params().markerfraction()
                                  });
            // create the servers
            auto sessionStatus = driver->GetSessionController()->StartServerAndConnect(config.bobaddress(),
                                                                  definedArguments.GetStringProp(Names::sessionAddr),
                                                                  static_cast<uint16_t>(config.sessionport()),
                                                                  LoadServerCredentials(creds));

            LogStatus(sessionStatus);
            if(sessionStatus.ok())
            {
                keystore = make_unique<keygen::KeyStore>(config.keylistenaddress(), LoadChannelCredentials(creds), config.bobaddress());

                grpc::ServerBuilder keyServBuilder;

                keyServBuilder.AddListeningPort(*config.mutable_keylistenaddress(), LoadServerCredentials(creds));
                keyServBuilder.RegisterService(this);

                keyServer = keyServBuilder.BuildAndStart();

                remote::OpticalParameters params;
                // TODO
                sessionStatus = LogStatus(driver->GetSessionController()->StartSession(params));
                if(!sessionStatus.ok())
                {
                    exitCode = ExitCodes::FailedToStartSession;
                    stopExecution = true;
                }
            } else {
                exitCode = ExitCodes::FailedToConnect;
                stopExecution = true;
            } // if session ok
        } // if driver valid

    } // if !Stop execution

    if(!stopExecution)
    {
        AddSignalHandler(SIGTERM, [this](int signum) {
            StopProcessing(signum);
        });
        // Wait for something to stop the session
        driver->GetSessionController()->WaitForEndOfSession();
    }
    return exitCode;
}

void HandheldAlice::StopProcessing(int)
{
    // The program is terminating,
    // stop the session
    driver->GetSessionController()->EndSession();
}

grpc::Status HandheldAlice::GetKeyStores(grpc::ServerContext*, const google::protobuf::Empty*, remote::SiteList* response)
{
    response->add_urls(config.bobaddress());
    return grpc::Status();
}

grpc::Status HandheldAlice::GetSharedKey(grpc::ServerContext*, const remote::KeyRequest* request, remote::SharedKey* response)
{
    grpc::Status result;
    if(request->siteto() == config.bobaddress())
    {
        // there is a keystore
        KeyID keyId = 0;
        PSK keyValue;

        // grpc proto syntax version 3: us oneof to do optional parameters
        if(request->opt_case() == remote::KeyRequest::OptCase::kKeyId)
        {
            // the request has included a keyid, get an existing key
            keyId = request->keyid();
            result = keystore->GetExistingKey(keyId, keyValue);
        }
        else
        {
            // get an unused key
            if(!keystore->GetNewKey(keyId, keyValue))
            {
                result = grpc::Status(grpc::StatusCode::RESOURCE_EXHAUSTED, "No key available");
            }
        } // if keyid specified

        if(result.ok())
        {
            // store the data in the outgoing variable
            response->set_keyid(keyId);
            response->mutable_keyvalue()->assign(keyValue.begin(), keyValue.end());

            // store a url on how to access the key
            response->set_url(KeyToPKCS11(keyId, config.bobaddress()));
        }
    } else {
        result = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid destination");
    }

    return result;
}

CQP_MAIN(HandheldAlice)

