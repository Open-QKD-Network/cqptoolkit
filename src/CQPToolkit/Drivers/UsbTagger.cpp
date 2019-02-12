/*!
* @file
* @brief CQP Toolkit - Usb Tagger
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Drivers/UsbTagger.h"
#include <libusb-1.0/libusb.h>       // for libusb_exit, libusb_free_device_...
#include "Algorithms/Logging/Logger.h"  // for LOGDEBUG, LOGERROR, LOGTRACE
#include "CQPToolkit/Drivers/Usb.h"
#include "CQPToolkit/Drivers/Serial.h"

namespace cqp
{
    using namespace std;

    void UsbTagger::ReadCallback(::libusb_transfer * transfer)
    {
        if (transfer != nullptr)
        {
            LOGTRACE(std::to_string(*transfer->buffer));
        }
    }

    UsbTagger::UsbTagger(const string& controlName, const string& usbSerialNumber)
    {
        if(controlName.empty())
        {
            SerialList devices;
            Serial::Detect(devices, true);
            if(devices.empty())
            {
                LOGERROR("No serail device found");
            } else {
                configPort = move(devices[0]);
            }
        } else {
            configPort = std::make_unique<Serial>(controlName, myBaudRate);
        }

        dataPort = Usb::Detect(usbVID, usbPID, usbSerialNumber);

    }

    UsbTagger::UsbTagger(std::unique_ptr<Serial> controlDev, std::unique_ptr<Usb> dataDev) :
        configPort{move(controlDev)}, dataPort{move(dataDev)}
    {

    }

    UsbTagger::~UsbTagger() = default;

    grpc::Status UsbTagger::StartDetecting(grpc::ServerContext*, const google::protobuf::Timestamp* request, google::protobuf::Empty*)
    {
        grpc::Status result;
        if(configPort)
        {
            configPort->Write('R');
            // TODO: This need to be on a separate thread and presumably keep going until StopDetecting is called
            dataPort->StartReadingBulk(&UsbTagger::ReadTags, BulkReadRequest, this);
        } else {
            result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Invalid serial device");
        }
        return result;
    }

    grpc::Status UsbTagger::StopDetecting(grpc::ServerContext*, const google::protobuf::Timestamp* request, google::protobuf::Empty*)
    {
        grpc::Status result;
        if(configPort)
        {
            configPort->Write('S');
        } else {
            result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Invalid serial device");
        }
        return result;
    }

    bool UsbTagger::Initialise()
    {
        bool result = true;

        if (configPort != nullptr)
        {
            const char InitSeq[] = { 'W', 'S' };
            // predefined command sequence for initialisation
            for (char cmd : InitSeq)
            {
                result &= configPort->Write(cmd);
                this_thread::sleep_for(chrono::seconds(1));
            }
        }

        return result;
    }

    URI UsbTagger::GetAddress()
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

    void UsbTagger::ReadTags(libusb_transfer* transfer)
    {
        UsbTagger* self = reinterpret_cast<UsbTagger*>(transfer->user_data);
        if(self)
        {
            // TODO


            //self->myStats.qubitsReceived.Update(...)
        }
    }

}
