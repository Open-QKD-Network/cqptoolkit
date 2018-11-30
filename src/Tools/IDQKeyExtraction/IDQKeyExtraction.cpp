/*!
* @file
* @brief IDQKeyExtraction
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "IDQKeyExtraction.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPAlgorithms/Util/Strings.h"
#include "QKDInterfaces/IIDQWrapper.grpc.pb.h"
#include <iostream>
#include <iomanip>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "CQPToolkit/Auth/AuthUtil.h"

using namespace cqp;

CQP_MAIN(IDQKeyExtraction)

IDQKeyExtraction::IDQKeyExtraction()
{
    using std::placeholders::_1;
    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);


    definedArguments.AddOption("help", "h", "Display help information on command line arguments")
    .Callback(std::bind(&IDQKeyExtraction::HandleHelp, this, _1));

    definedArguments.AddOption("verbose", "v", "Increase output")
    .Callback(std::bind(&IDQKeyExtraction::HandleVerbose, this, _1));

    definedArguments.AddOption("quiet", "q", "Decrease output")
    .Callback(std::bind(&IDQKeyExtraction::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::local, "a", "Local wrapper address")
    .Required()
    .Bind();

    definedArguments.AddOption(Names::remote, "r", "Remote wrapper internal address")
    .Bind();

    definedArguments.AddOption(Names::internalPort, "i", "Remote wrapper internal port")
    .Bind();

    definedArguments.AddOption(Names::lineAtten, "l", "Line attenuation")
    .Bind();

    definedArguments.AddOption(Names::initPsk, "k", "Initial pre-shared key as 32byte hex")
    .Bind();

    definedArguments.AddOption(Names::CertFile, "", "Certificate file")
    .Bind();

    definedArguments.AddOption(Names::KeyFile, "", "Key file")
    .Bind();

    definedArguments.AddOption(Names::RootCaFile, "", "Root Certificate file")
    .Bind();
    definedArguments.AddOption(Names::Tls, "s", "Use secure connections");
}

int IDQKeyExtraction::Main(const std::vector<std::string>& args)
{
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(Names::CertFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::KeyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::RootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(Names::Tls))
        {
            creds.set_usetls(true);
        }

        auto channel = grpc::CreateChannel(definedArguments.GetStringProp(Names::local), LoadChannelCredentials(creds));
        if(channel)
        {
            auto stub = remote::IIDQWrapper::NewStub(channel);
            if(stub)
            {
                grpc::ClientContext detailsCtx;
                google::protobuf::Empty detailsRequest;
                remote::WrapperDetails wrapperDetails;

                grpc::Status getDetailsStatus = LogStatus(
                                                    stub->GetDetails(&detailsCtx, detailsRequest, &wrapperDetails));

                if(getDetailsStatus.ok())
                {
                    LOGINFO("Connected to wrapper on: " + definedArguments.GetStringProp(Names::local) +
                            " with internal name address: " +
                            wrapperDetails.hostname() + ":" + std::to_string(wrapperDetails.portnumber()));

                    using namespace std;
                    grpc::ClientContext startCtx;
                    remote::IDQStartOptions startOptions;

                    {
                        definedArguments.GetProp(Names::remote, *startOptions.mutable_peerhostname());
                        double lineAtten = 3.0;
                        definedArguments.GetProp(Names::lineAtten, lineAtten);
                        startOptions.set_lineattenuation(lineAtten);
                        int internalPort = 7000;
                        definedArguments.GetProp(Names::internalPort, internalPort);
                        startOptions.set_peerwrapperport(internalPort);

                        std::string initPskHex = "af21a0ac8f827d51a961a5552c37aac286a42d3d854ae84680c0e136a7ccc7d0";
                        definedArguments.GetProp(Names::initPsk, initPskHex);
                        auto initPsk = HexToBytes(initPskHex);
                        startOptions.mutable_initialsecret()->assign(initPsk.begin(), initPsk.end());
                    }

                    LOGTRACE("Calling StartQKDSequence");
                    auto reader = stub->StartQKDSequence(&startCtx, startOptions);
                    remote::SharedKey sharedKey;

                    LOGTRACE("Waiting for key...");
                    while(reader && reader->Read(&sharedKey))
                    {
                        std::stringstream strm;
                        strm << "Key " << uppercase << std::hex << sharedKey.keyid() << " = ";

                        for(uint8_t byte : sharedKey.keyvalue())
                        {
                            // the "+" forces the byte to be read as a number
                            strm << setw(2) << setfill('0') << uppercase << hex << +byte;
                        }
                        LOGDEBUG("Got key message: " + strm.str());
                        sharedKey.Clear();
                    }

                    if(reader)
                    {
                        LogStatus(reader->Finish());
                    }

                    LOGTRACE("StartQKDSequence finished");
                }
            }
            else
            {
                LOGERROR("Failed to connect to " + definedArguments.GetStringProp(Names::local));
            }
        }
        else
        {
            LOGERROR("Failed to create channel to " + definedArguments.GetStringProp(Names::local));
        }
    }

    return 0;
}

void IDQKeyExtraction::HandleHelp(const cqp::CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Extracts key from the IDQuantique Clavis 2.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}
