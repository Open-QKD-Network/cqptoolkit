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
#include <sys/types.h>               // for ssize_t
#include <string>                    // for string, operator+, to_string
#include "CQPToolkit/Util/Logger.h"  // for LOGDEBUG, LOGERROR, LOGTRACE
#include "Drivers/Usb.h"             // for Usb
#include "QKDInterfaces/Site.pb.h"   // for Device, Side_Type, Side_Type_Bob

namespace cqp
{
    class ISessionController;
    using namespace std;

    const std::string DriverName = "TaggerMk1";

    UsbTagger::UsbTagger()
    {
        configIndex = 1;
    }


    void UsbTagger::ReadCallback(libusb_transfer * transfer)
    {
        if (transfer != nullptr)
        {
            LOGTRACE(std::to_string(*transfer->buffer));
        }
    }

    bool UsbTagger::Open()
    {
        return Usb::Open(usbVid, usbPid, configIndex, interfaceIndex);
    }

    void UsbTagger::DetectFunc(UsbTaggerList& results, bool firstOnly)
    {
        libusb_device **devs;
        // Initialize the library.
        if (libusb_init(nullptr) != LIBUSB_SUCCESS)
        {
            LOGERROR("Failed to initialise libUSB.");
        }
        // Find all connected devices.
        ssize_t numDevices = libusb_get_device_list(nullptr, &devs);
        LOGDEBUG("Found " + std::to_string(numDevices) + " usb devices.");

        // Create an object which will hopfully connect
        UsbTagger* newDevice = new UsbTagger();
        for (int devIndex = 0; devIndex < numDevices; devIndex++)
        {
            if (newDevice->Open(devs[devIndex]))
            {
                LOGDEBUG("Found matching vid/pid...");
                // Success, store the new device and create a blank one to work with
                results.push_back(newDevice);
                newDevice = new UsbTagger();

                if (firstOnly)
                {
                    // Only new one device, stop looking
                    break; // for loop
                }
            }
        }
        // clean up any unused objects
        delete(newDevice);

        libusb_free_device_list(devs, 1);
        libusb_exit(nullptr);
    }

    string UsbTagger::GetDriverName() const
    {
        return DriverName;
    }

    bool UsbTagger::Initialise()
    {
        bool result = false;

        if (IsOpen())
        {
            StartReadingBulk(BulkReadRequest);
            result = true;
        }
        return result;
    }

    cqp::URI UsbTagger::GetAddress() const
    {
        return "";
    }

    std::string UsbTagger::GetDescription() const
    {
        return "";
    }

    ISessionController* UsbTagger::GetSessionController()
    {
        return nullptr;
    }

    remote::Device UsbTagger::GetDeviceDetails()
    {
        remote::Device result;
        // TODO
        result.set_side(remote::Side_Type::Side_Type_Bob);

        return result;
    }

}
