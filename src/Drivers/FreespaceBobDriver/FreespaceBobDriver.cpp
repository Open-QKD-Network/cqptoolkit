/*!
* @file
* @brief FreespaceBobDriver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "FreespaceBobDriver.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"
#include "CQPToolkit/QKDDevices/LEDAliceMk1.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "CQPToolkit/QKDDevices/PhotonDetectorMk1.h"

using namespace cqp;
using namespace std;

struct FreespaceBobDriver::FreespaceNames
{
    static CONSTSTRING device = "device";
    static CONSTSTRING usbDevice = "usb-device";
    static CONSTSTRING writeConfig = "write-config";
};

FreespaceBobDriver::FreespaceBobDriver() :
    DriverApplication()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    // attach the sub-config to the master config
    config.set_allocated_controlparams(controlDetails);

    definedArguments.AddOption(FreespaceNames::device, "d", "The serial device to use, otherwise the first serial device will be used")
    .Bind();
    definedArguments.AddOption(FreespaceNames::usbDevice, "u", "The serial number for the usb device to use, otherwise use the first detected")
    .Bind();
    definedArguments.AddOption(FreespaceNames::writeConfig, "", "Output the resulting config to a file")
    .Bind();

}

FreespaceBobDriver::~FreespaceBobDriver()
{
    adaptor.reset();
    device.reset();
}

void FreespaceBobDriver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);

}

int FreespaceBobDriver::Main(const std::vector<std::string>& args)
{
    exitCode = DriverApplication::Main(args);

    if(!stopExecution)
    {
        definedArguments.GetProp(FreespaceNames::device, *config.mutable_devicename());
        definedArguments.GetProp(FreespaceNames::usbDevice, *config.mutable_usbdevicename());

        // any changes to config need to be applied by now
        if(definedArguments.HasProp(FreespaceNames::writeConfig))
        {
            // write out the current settings
            WriteConfigFile(config, definedArguments.GetStringProp(FreespaceNames::writeConfig));
        } // if write config file

        device = make_shared<PhotonDetectorMk1>(channelCreds, config.devicename(), config.usbdevicename());
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

        LOGINFO("My device id is " + config.controlparams().config().id());

        // Wait for something to stop the driver
        adaptor->WaitForServerShutdown();
    }
    return exitCode;
}

void FreespaceBobDriver::StopProcessing(int)
{
    // The program is terminating,
    ShutdownNow();
    device.reset();
}

CQP_MAIN(FreespaceBobDriver)
