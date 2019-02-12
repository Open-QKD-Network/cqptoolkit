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
};

const std::string defaultKeyServAddress = "localhost:9001";


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


    definedArguments.AddOption(Names::keyServer, "k", "Listen address for serving keys, defaults to " + defaultKeyServAddress)
    .Bind();
    definedArguments.AddOption(Names::sessionAddr, "", "Bind address for internal communication")
    .Bind();
    definedArguments.AddOption(Names::sessionPort, "", "Port for internal communication")
    .Bind();
    definedArguments.AddOption(Names::device, "d", "The serial device to use, otherwise the first serial device will be used")
    .Bind();
    definedArguments.AddOption(Names::usbDevice, "u", "The serial number for the usb device to use, otherwise use the first detected")
    .Bind();
    definedArguments.AddOption(Names::connect, "c", "The address of Bob's session controller. Required")
    .Bind().Required();
}

void HandheldAlice::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Outputs statistics from CQP services in CSV format.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
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

        std::string keyListenAddress = defaultKeyServAddress;
        std::string sessionAddress;
        std::string deviceName;
        std::string usbDeviceName;
        definedArguments.GetProp(Names::keyServer, keyListenAddress);
        definedArguments.GetProp(Names::sessionAddr, sessionAddress);
        definedArguments.GetProp(Names::device, deviceName);
        definedArguments.GetProp(Names::usbDevice, usbDeviceName);

        driver = make_shared<LEDAliceMk1>(LoadChannelCredentials(creds), deviceName, usbDeviceName);

        if(driver)
        {
            // create the servers
            uint16_t sessionPort = 0;
            definedArguments.GetProp(Names::sessionPort, sessionPort);
            keystoreUrl = definedArguments.GetStringProp(Names::connect);

            auto sessionStatus = driver->GetSessionController()->StartServerAndConnect(keystoreUrl,
                                                                  definedArguments.GetStringProp(Names::sessionAddr),
                                                                  sessionPort,
                                                                  LoadServerCredentials(creds));

            LogStatus(sessionStatus);
            if(sessionStatus.ok())
            {
                keystore = make_unique<keygen::KeyStore>(keyListenAddress, LoadChannelCredentials(creds), keystoreUrl);

                grpc::ServerBuilder keyServBuilder;

                keyServBuilder.AddListeningPort(keyListenAddress, LoadServerCredentials(creds));
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
    response->add_urls(keystoreUrl);
    return grpc::Status();
}

grpc::Status HandheldAlice::GetSharedKey(grpc::ServerContext*, const remote::KeyRequest* request, remote::SharedKey* response)
{
    grpc::Status result;
    if(request->siteto() == keystoreUrl)
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
            response->set_url(KeyToPKCS11(keyId, keystoreUrl));
        }
    } else {
        result = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid destination");
    }

    return result;
}

CQP_MAIN(HandheldAlice)

