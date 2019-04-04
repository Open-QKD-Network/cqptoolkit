/*!
* @file
* @brief Clavis3Driver
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "Clavis3Driver.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "IDQDevices/Clavis3/Clavis3Device.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/Statistics/ReportServer.h"

using namespace cqp;

Clavis3Driver::Clavis3Driver() :
    reportServer{std::make_shared<stats::ReportServer>()}
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    definedArguments.AddOption(Names::device, "d", "Device address")
    .Bind();
    definedArguments.AddOption(Names::manual, "m", "Manual mode, specify Bobs address to directly connect and start generating key")
    .Bind();

    definedArguments.AddOption(Names::writeConfig, "", "Output the resulting config to a file")
    .Bind();
}

Clavis3Driver::~Clavis3Driver()
{
    adaptor.reset();
    device.reset();
}

void Clavis3Driver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);
}

int Clavis3Driver::Main(const std::vector<std::string> &args)
{
    DriverApplication::Main(args);
    if(!stopExecution)
    {
        config.set_allocated_controlparams(controlDetails);

        definedArguments.GetProp(Names::device, *config.mutable_deviceaddress());
        if(config.deviceaddress().empty())
        {
            LOGERROR("Device address required");
            stopExecution = true;
            exitCode = ExitCodes::InvalidConfig;
        }
    }

    if(!stopExecution)
    {
        using namespace std;

        device = make_shared<Clavis3Device>(config.deviceaddress(), channelCreds, reportServer);
        adaptor = make_unique<RemoteQKDDevice>(device, serverCreds);

        // get the real settings which have been corrected by the device driuver
        (*config.mutable_controlparams()->mutable_config()) = device->GetDeviceDetails();

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(Names::writeConfig))
        {
            // write out the current settings
            WriteConfigFile(config, definedArguments.GetStringProp(Names::writeConfig));
        } // if write config file

        stopExecution = ! adaptor->StartControlServer(config.controlparams().controladdress(), config.controlparams().siteagentaddress());

    }

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

void Clavis3Driver::StopProcessing(int)
{
    // The program is terminating,
    ShutdownNow();
    device.reset();
}

CQP_MAIN(Clavis3Driver)
