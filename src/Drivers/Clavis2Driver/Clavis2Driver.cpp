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
#include "IDQDevices/Clavis2/ClavisProxy.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"

using namespace cqp;
using namespace std;

struct Clavis2Driver::Clavis2Names
{
    static CONSTSTRING manual = "manual";
    static CONSTSTRING writeConfig = "write-config";
    static CONSTSTRING attenuation = "atteniation";
};

Clavis2Driver::Clavis2Driver()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    // attach the sub-config to the master config
    config.set_allocated_controlparams(controlDetails);

    definedArguments.AddOption(Clavis2Names::manual, "m", "Manual mode, specify Bobs address to directly connect and start generating key")
    .Bind();
    definedArguments.AddOption(Clavis2Names::writeConfig, "", "Output the resulting config to a file")
    .Bind();
    definedArguments.AddOption(Clavis2Names::attenuation, "-a", "Line attenuation for manual mode")
    .Bind();

}

Clavis2Driver::~Clavis2Driver()
{
    adaptor.reset();
    device.reset();
}

void Clavis2Driver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);

}

int Clavis2Driver::Main(const std::vector<std::string>& args)
{
    exitCode = DriverApplication::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(Clavis2Names::manual, *config.mutable_bobaddress());

        device = make_shared<ClavisProxy>(config.controlparams().config(), channelCreds);
        adaptor = make_unique<RemoteQKDDevice>(device, serverCreds);
        // default key for manual mode
        auto authKey = std::make_unique<PSK>();
        authKey->assign({0xfd, 0x48, 0xf8, 0x4c, 0x6a, 0x19, 0xdf, 0xf1, 0x0d, 0xa2, 0x2a, 0xd0, 0x7c, 0x10, 0xa3, 0xf0,
                         0xfd, 0x48, 0xf8, 0x4c, 0x6a, 0x19, 0xdf, 0xf1, 0x0d, 0xa2, 0x2a, 0xd0, 0x7c, 0x10, 0xa3, 0xf0});
        device->SetInitialKey(move(authKey));
        device->Initialise(remote::SessionDetails());


        // get the real settings which have been corrected by the device driuver
        (*config.mutable_controlparams()->mutable_config()) = device->GetDeviceDetails();

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(Clavis2Names::writeConfig))
        {
            WriteConfigFile(config, definedArguments.GetStringProp(Clavis2Names::writeConfig));
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

        LOGINFO("My device id is " + config.controlparams().config().id());

        if(config.controlparams().config().side() == remote::Side_Type::Side_Type_Alice && !config.bobaddress().empty())
        {
            // TODO: get the key and do something with it
            grpc::ServerContext ctx;
            remote::SessionDetailsTo request;
            google::protobuf::Empty response;

            request.set_peeraddress(config.bobaddress());
            if(definedArguments.HasProp(Clavis2Names::attenuation))
            {
                double atten = 0.0;
                definedArguments.GetProp(Clavis2Names::attenuation, atten);
                request.mutable_details()->set_lineattenuation(atten);
            }
            LogStatus(adaptor->RunSession(&ctx, &request, &response));
        }

        // Wait for something to stop the driver
        WaitForShutdown();
    }
    return exitCode;
}

void Clavis2Driver::StopProcessing(int)
{
    // The program is terminating,
    ShutdownNow();
    device.reset();
}

CQP_MAIN(Clavis2Driver)
