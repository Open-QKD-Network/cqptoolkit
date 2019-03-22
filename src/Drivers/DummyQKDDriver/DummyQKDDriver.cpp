/*!
* @file
* @brief DummyQKDDriver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DummyQKDDriver.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/QKDDevices/DummyQKD.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"

using namespace cqp;
using namespace std;

struct DummyQKDDriver::DummyNames
{
    static CONSTSTRING alice = "alice";
    static CONSTSTRING bob = "bob";
    static CONSTSTRING manual = "manual";
    static CONSTSTRING writeConfig = "write-config";
};

DummyQKDDriver::DummyQKDDriver()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    // attach the sub-config to the master config
    config.set_allocated_controlparams(controlDetails);

    definedArguments.AddOption(DummyNames::alice, "a", "Set side to Alice.");
    definedArguments.AddOption(DummyNames::bob, "b", "Set side to Bob.");

    definedArguments.AddOption(DummyNames::manual, "m", "Manual mode, specify Bobs address to directly connect and start generating key")
    .Bind();

    definedArguments.AddOption(DummyNames::writeConfig, "", "Output the resulting config to a file")
    .Bind();
}

DummyQKDDriver::~DummyQKDDriver()
{
    adaptor.reset();
    device.reset();
}

void DummyQKDDriver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);
}

int DummyQKDDriver::Main(const std::vector<std::string>& args)
{
    exitCode = DriverApplication::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(DummyNames::manual, *config.mutable_bobaddress());

        if(definedArguments.IsSet(DummyNames::alice))
        {
            config.mutable_controlparams()->mutable_config()->set_side(remote::Side_Type::Side_Type_Alice);
        }

        if(definedArguments.IsSet(DummyNames::bob))
        {
            config.mutable_controlparams()->mutable_config()->set_side(remote::Side_Type::Side_Type_Bob);
        }

        device = make_shared<DummyQKD>(config.controlparams().config(), channelCreds);
        adaptor = make_unique<RemoteQKDDevice>(device, serverCreds);

        // get the real settings which have been corrected by the device driuver
        (*config.mutable_controlparams()->mutable_config()) = device->GetDeviceDetails();

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(DummyNames::writeConfig))
        {
            // write out the current settings
            WriteConfigFile(config, definedArguments.GetStringProp(DummyNames::writeConfig));
        } // if write config file

        stopExecution = ! adaptor->StartControlServer(config.controlparams().controladdress(), config.controlparams().siteagentaddress());

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

        if(config.controlparams().config().side() == remote::Side_Type::Side_Type_Alice && !config.bobaddress().empty())
        {
            // TODO: get the key and do something with it
            grpc::ServerContext ctx;
            remote::SessionDetailsTo request;
            google::protobuf::Empty response;

            request.set_peeraddress(config.bobaddress());
            LogStatus(adaptor->RunSession(&ctx, &request, &response));
        }

        // Wait for something to stop the driver
        WaitForShutdown();
    }
    return exitCode;
}

void DummyQKDDriver::StopProcessing(int)
{
    // The program is terminating,
    ShutdownNow();
    device.reset();
}

CQP_MAIN(DummyQKDDriver)
