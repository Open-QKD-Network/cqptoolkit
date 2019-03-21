/*!
* @file
* @brief HandheldDriver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "HandheldDriver.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/QKDDevices/LEDAliceMk1.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"

using namespace cqp;
using namespace std;

struct HandheldDriver::HandheldNames
{
    static CONSTSTRING device = "device";
    static CONSTSTRING usbDevice = "usb-device";
    static CONSTSTRING manual = "manual";
    static CONSTSTRING writeConfig = "write-config";
};

HandheldDriver::HandheldDriver() :
    DriverApplication()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    // attach the sub-config to the master config
    config.set_allocated_controlparams(controlDetails);

    definedArguments.AddOption(HandheldNames::device, "d", "The serial device to use, otherwise the first serial device will be used")
    .Bind();
    definedArguments.AddOption(HandheldNames::usbDevice, "u", "The serial number for the usb device to use, otherwise use the first detected")
    .Bind();
    definedArguments.AddOption(HandheldNames::manual, "m", "Manual mode, specify Bobs address to directly connect and start generating key")
    .Bind();
    definedArguments.AddOption(HandheldNames::writeConfig, "", "Output the resulting config to a file")
    .Bind();

}

HandheldDriver::~HandheldDriver()
{
    adaptor.reset();
    device.reset();
}

void HandheldDriver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);

}

int HandheldDriver::Main(const std::vector<std::string>& args)
{
    exitCode = DriverApplication::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(HandheldNames::manual, *config.mutable_bobaddress());
        definedArguments.GetProp(HandheldNames::device, *config.mutable_devicename());
        definedArguments.GetProp(HandheldNames::usbDevice, *config.mutable_usbdevicename());

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(HandheldNames::writeConfig))
        {
            // write out the current settings
            WriteConfigFile(config, definedArguments.GetStringProp(HandheldNames::writeConfig));
        } // if write config file

        device = make_shared<LEDAliceMk1>(channelCreds, config.devicename(), config.usbdevicename());
        adaptor = make_unique<RemoteQKDDevice>(device, serverCreds);

        // get the real settings which have been corrected by the device driuver
        (*config.mutable_controlparams()->mutable_config()) = device->GetDeviceDetails();

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

        if(!config.bobaddress().empty())
        {
            // TODO: get the key and do something with it
            grpc::ServerContext ctx;
            remote::SessionDetailsTo request;
            google::protobuf::Empty response;

            request.set_peeraddress(config.bobaddress());
            LogStatus(adaptor->RunSession(&ctx, &request, &response));
        }

        // Wait for something to stop the driver
        adaptor->WaitForServerShutdown();
    }
    return exitCode;
}

void HandheldDriver::StopProcessing(int)
{
    // The program is terminating,
    adaptor->StopServer();
}

CQP_MAIN(HandheldDriver)
