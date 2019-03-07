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

#include "FreespaceTest.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include <iostream>
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Util/FileIO.h"
#include "QKDInterfaces/Device.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Drivers/Usb.h"

using namespace cqp;

struct Names
{
    static CONSTSTRING device = "device";
    static CONSTSTRING usbDevice = "usb-device";
    static CONSTSTRING alice = "alice";
    static CONSTSTRING bob = "bob";
    static CONSTSTRING output = "output";
    static CONSTSTRING numPhotons = "num-photons";
    static CONSTSTRING timeout = "timeout";
};

FreespaceTest::FreespaceTest()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&FreespaceTest::DisplayHelp, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&FreespaceTest::HandleQuiet, this, _1));

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&FreespaceTest::HandleVerbose, this, _1));
    definedArguments.AddOption(Names::device, "d", "The serial device to use, otherwise the first serial device will be used")
    .Bind();
    definedArguments.AddOption(Names::usbDevice, "u", "The serial number for the usb device to use, otherwise use the first detected")
    .Bind();

    definedArguments.AddOption(Names::alice, "a", "Alice mode, generate random qubits and transmit them");
    definedArguments.AddOption(Names::bob, "b", "Bob mode, detect qubits and store them");
    definedArguments.AddOption(Names::output, "o", "Output file for the results")
            .Bind();
    definedArguments.AddOption(Names::numPhotons, "n", "Alice: Number of photons to transmit")
            .Bind();
    definedArguments.AddOption(Names::timeout, "t", "Timeout for detections in miliseconds")
            .Bind();
}

void FreespaceTest::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Directly drives handheld Alice or freespace Bob for testing.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void FreespaceTest::HandleConfigFile(const CommandArgs::Option& option)
{
    if(fs::Exists(option.value))
    {
        std::string buffer;
        if(fs::ReadEntireFile(option.value, buffer))
        {
            using namespace google::protobuf::util;
            JsonParseOptions parseOptions;
            /*if(!LogStatus(JsonStringToMessage(buffer, &config, parseOptions)).ok())
            {
                exitCode = InvalidConfig;
            }
            else if(!config.bobaddress().empty())
            {
                // dont need this option if the config file specifies it
                definedArguments[Names::connect]->set = true;
            }*/
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

int FreespaceTest::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        string serialDevice;
        string usbSerialNum;

        definedArguments.GetProp(Names::device, serialDevice);
        definedArguments.GetProp(Names::usbDevice, usbSerialNum);

        AddSignalHandler(SIGTERM, [this](int signum) {
            StopProcessing(signum);
        });
        AddSignalHandler(SIGINT, [this](int signum) {
            StopProcessing(signum);
        });

        remote::DeviceConfig params;

        if(definedArguments.HasProp(Names::alice))
        {
            LOGINFO("Running in Alice mode. Output will contain random bytes transmitted");

            string outputFilename = "alice.csv";
            definedArguments.GetProp(Names::output, outputFilename);
            try {
                outputFile.open(outputFilename, std::ofstream::out);
            } catch (const exception& e) {
                LOGERROR(e.what());
                exitCode = ExitCodes::InvalidConfig;
                stopExecution = true;
            }

            leds = make_unique<LEDDriver>(&rng, serialDevice, usbSerialNum);

            if(leds)
            {
                uint64_t numPhotons = 0;
                if(definedArguments.GetProp(Names::numPhotons, numPhotons))
                {
                    leds->SetPhotonsPerBurst(numPhotons);
                }
                leds->Attach(this);
                if(leds->Initialise(params))
                {
                    leds->StartFrame();
                    leds->Fire();
                    leds->EndFrame();
                } else
                {
                    LOGERROR("Failed to initialise device");
                    exitCode = ExitCodes::InvalidConfig;
                }
            } else {
                LOGERROR("Failed to create device");
                exitCode = ExitCodes::NoDevice;
                stopExecution = true;
            }

        } else {
            LOGINFO("Running in Bob mode. Output will be: picoseconds,channel");
            string outputFilename = "bob.csv";
            definedArguments.GetProp(Names::output, outputFilename);
            try {
                outputFile.open(outputFilename, std::ofstream::out);

            } catch (const exception& e) {
                LOGERROR(e.what());
                exitCode = ExitCodes::InvalidConfig;
                stopExecution = true;
            }

            tagger = make_unique<UsbTagger>(serialDevice, usbSerialNum);
            if(tagger)
            {
                tagger->Attach(this);
                if(tagger->Initialise(params))
                {
                    LOGINFO("Detecting...");
                    google::protobuf::Timestamp request;
                    google::protobuf::Empty response;
                    if(LogStatus(tagger->StartDetecting(nullptr, &request, &response)).ok())
                    {
                        unique_lock<mutex> lock(waitMutex);
                        int timeout(0);
                        if(definedArguments.HasProp(Names::timeout) && definedArguments.GetProp(Names::timeout, timeout))
                        {
                            this_thread::sleep_for(std::chrono::milliseconds(timeout));
                            tagger->StopDetecting(nullptr, nullptr, nullptr);
                            waitCv.wait(lock, [&]{
                                return stopExecution;
                            });
                        } else {
                            waitCv.wait(lock, [&]{
                                return stopExecution;
                            });
                        }
                    } else {
                        LOGERROR("Failed to start detecting");
                        exitCode = ExitCodes::UnknownError;
                    }
                } else {
                    LOGERROR("Failed to intialise device");
                    exitCode = ExitCodes::InvalidConfig;
                }
            } else {
                LOGERROR("Failed to create tagger");
                exitCode = ExitCodes::NoDevice;
            }
        }
    }

    if(exitCode != ExitCodes::Ok)
    {
        stopExecution = true;
    }

    return exitCode;
}

void FreespaceTest::StopProcessing(int)
{
    LOGTRACE("");
    using namespace std;
    // The program is terminating,
    // stop the session
    if(tagger)
    {
        google::protobuf::Timestamp request;
        google::protobuf::Empty response;
        if(LogStatus(tagger->StopDetecting(nullptr, &request, &response)).ok())
        {
            unique_lock<mutex> lock(waitMutex);
            waitCv.wait(lock, [&]{
                return stopExecution;
            });
        }
    }
    exit(exitCode);
}


void FreespaceTest::OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)
{
    using namespace std;
    if(outputFile.is_open())
    {
        for(const auto& detection : report->detections)
        {
            outputFile << detection.time.count() << ", " << std::to_string(detection.value) << endl;
        }
        outputFile.close();
    } else {
        LOGWARN("Output file not writabe");
    }
    stopExecution = true;
    waitCv.notify_all();
}

void FreespaceTest::OnEmitterReport(std::unique_ptr<EmitterReport> report)
{
    using namespace std;
    if(outputFile.is_open())
    {
        for(const auto& emision : report->emissions)
        {
            outputFile << emision << endl;
        }
        outputFile.close();
    } else {
        LOGWARN("Output file not writabe");
    }
    stopExecution = true;
    waitCv.notify_all();
}

CQP_MAIN(FreespaceTest)
