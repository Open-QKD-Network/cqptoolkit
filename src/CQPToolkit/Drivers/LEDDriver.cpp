/*!
* @file
* @brief CQP Toolkit - LED Driver board
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "LEDDriver.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Drivers/Usb.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "Algorithms/Random/IRandom.h"
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{

    LEDDriver::LEDDriver(IRandom* randomSource, const std::string& controlName, const std::string& usbSerialNumber):
        randomness{randomSource}
    {
        if(controlName.empty())
        {
            SerialList devices;
            Serial::Detect(devices, true);
            if(devices.empty())
            {
                LOGERROR("No serial device found");
            } else {
                configPort = move(devices[0]);
            }
        } else {
            configPort = std::make_unique<Serial>(controlName, myBaudRate);
        }

        dataPort = Usb::Detect(usbVID, usbPID, usbSerialNumber);

    }

    LEDDriver::LEDDriver(IRandom* randomSource, std::unique_ptr<Serial> controlDev, std::unique_ptr<Usb> dataDev):
        dataPort{move(dataDev)},
        configPort{move(controlDev)},
        randomness{randomSource}
    {

    }

    bool LEDDriver::Fire(unsigned long numQubits)
    {
        using namespace std;
        bool result = false;
        if(dataPort)
        {
            const auto QubitsPerByte = sizeof(uint8_t)*8 / bitsPerQubit;
            auto bytesToSend = numQubits / QubitsPerByte;

            if((numQubits % QubitsPerByte) != 0)
            {
                bytesToSend++;
            }

            auto report = std::make_unique<EmitterReport>();
            report->emissions.reserve(bytesToSend);
            report->epoc = epoc;
            report->frame = frame;
            // TODO: report->period

            // generate the random values
            randomness->RandomBytes(bytesToSend, report->emissions);
            // send them to the device
            result = dataPort->WriteBulk(report->emissions, UsbEndpoint);
            if(result)
            {
                // pass the random values onto the processing chain
                Emit(&IEmitterEventCallback::OnEmitterReport, move(report));
            }
        }
        return result;
    }


    void LEDDriver::StartFrame()
    {
        using std::chrono::high_resolution_clock;
        epoc = high_resolution_clock::now();
    }

    void LEDDriver::EndFrame()
    {
        using std::chrono::high_resolution_clock;
        myStats.frameTime.Update(high_resolution_clock::now() - epoc);
        frame++;
    }

    URI LEDDriver::GetAddress() const
    {
        URI result;
        std::string hostString = fs::BaseName(configPort->GetAddress().GetPath());
        for(auto element : dataPort->GetPortNumbers())
        {
            hostString += "-" + std::to_string(element);
        }
        result.SetHost(hostString);
        result.SetParameter(Parameters::serial, configPort->GetAddress().GetPath());
        result.SetParameter(Parameters::usbserial, dataPort->GetSerialNumber());

        return result;
    }

    bool LEDDriver::Initialise(remote::DeviceConfig& parameters)
    {
        using std::chrono::nanoseconds;
        //std::cout << "\tSending " << Div << ", " << Del << std::endl;
        bool result = false;
        if(configPort && dataPort)
        {
            result = dataPort->Open();
        }

        if(result)
        {
            result = configPort->Write(divEndpoint);
            //std::this_thread::sleep_for(nanoseconds(1));
            result &= configPort->Write(div10Mhz);
            //std::this_thread::sleep_for(nanoseconds(1));
            result &= configPort->Write(commandEnd);
            //std::this_thread::sleep_for(nanoseconds(1));
            result &= configPort->Write(delEndPoint);
            //std::this_thread::sleep_for(nanoseconds(1));
            result &= configPort->Write(del7ns);
            //std::this_thread::sleep_for(nanoseconds(1));
            result &= configPort->Write(commandEnd);
            //std::this_thread::sleep_for(nanoseconds(1));
        }
        return result;
    }

}
