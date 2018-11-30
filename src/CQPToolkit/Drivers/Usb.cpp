/*!
* @file
* @brief CQP Toolkit - USB IO
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Drivers/Usb.h"
#include <libusb-1.0/libusb.h>       // for LIBUSB_SUCCESS, libusb_exit, lib...
#include <cstddef>                   // for size_t
#include <sys/types.h>               // for ssize_t
#include <string>                    // for operator+, to_string, char_traits
#include <unordered_map>             // for unordered_map
#include "CQPAlgorithms/Logging/Logger.h"  // for LOGTRACE, LOGERROR, DefaultLogger
#include "CQPAlgorithms/Datatypes/Base.h"          // for DataBlock, IntList
#include "CQPAlgorithms/Logging/ILogger.h"      // for LogLevel, ILogger, LogLevel::Debug
           // for EnumClassHash
#include "CQPToolkit/Util/URI.h"                // for URI

namespace cqp
{
    using namespace std;
#if __GNUC__ < 7
    /// Used for mapping log levels to libusb
    using UsbLevelMap = std::unordered_map<LogLevel, libusb_log_level, EnumClassHash> ;
#else
    /// Used for mapping log levels to libusb
    using UsbLevelMap = std::unordered_map<LogLevel, libusb_log_level> ;
#endif

    /// Used for mapping log levels to libusb
    /// @returns populated map
    static UsbLevelMap CreateUsbLevelMap()
    {
        UsbLevelMap result;
        result[LogLevel::Debug] = libusb_log_level::LIBUSB_LOG_LEVEL_DEBUG;
        result[LogLevel::Error] = libusb_log_level::LIBUSB_LOG_LEVEL_ERROR;
        result[LogLevel::Info] = libusb_log_level::LIBUSB_LOG_LEVEL_INFO;
        result[LogLevel::Trace] = libusb_log_level::LIBUSB_LOG_LEVEL_DEBUG;
        result[LogLevel::Warning] = libusb_log_level::LIBUSB_LOG_LEVEL_WARNING;
        return result;
    }
    /// Used for mapping log levels to libusb
    const UsbLevelMap UsbLevelLookup = CreateUsbLevelMap();


    Usb::Usb()
    {
        // Initialize the library.
        // by passing null we are using the default context which is reference counted
        if (LogUsb(libusb_init(nullptr)) == LIBUSB_SUCCESS)
        {
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000106)
            libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, UsbLevelLookup.at(DefaultLogger().GetOutputLevel()));
#else
            libusb_set_debug(nullptr, UsbLevelLookup.at(DefaultLogger().GetOutputLevel()));
#endif
        }
        else
        {
            LOGERROR("Failed to initialise libUSB.");
        }

    }

    Usb::~Usb()
    {
        Usb::Close();
    }

    bool Usb::IsOpen() const
    {
        return (myDev != nullptr && myHandle != nullptr);
    }

    bool Usb::Open(uint16_t vendorId, uint16_t productId, int newConfigIndex, int newInterfaceIndex)
    {
        vendor = vendorId;
        product = productId;
        LOGTRACE("Getting device descriptor...");
        configIndex = newConfigIndex;
        interfaceIndex = newInterfaceIndex;
        bool result = false;

        myHandle = libusb_open_device_with_vid_pid(nullptr, vendorId, productId);
        if(myHandle != nullptr)
        {
            myDev = libusb_get_device(myHandle);
        }

        /// The description of the connected device
        struct libusb_device_descriptor desc = libusb_device_descriptor();

        if (myDev == nullptr || LogUsb(libusb_get_device_descriptor(myDev, &desc)) != LIBUSB_SUCCESS)
        {
            LOGERROR("failed to get device descriptor");
        }
        else
        {
            LOGTRACE("Device has " + std::to_string(desc.bNumConfigurations) + " Configurations.");
        }

        result = (desc.idVendor == vendorId) &&
                 (desc.idProduct == productId);

        if (result)
        {
            LOGTRACE("Opening Device...");
            // Now try to get a handle to the device
            result = LogUsb(libusb_open(myDev, &myHandle)) == LIBUSB_SUCCESS;
            Usb::Open();
        }

        return result;
    }

    void Usb::ReadCallback(libusb_transfer *)
    {
        LOGTRACE("ReadCallback called");
    }

    bool Usb::Open()
    {
        bool result = true;

        if (myDev != nullptr && myHandle != nullptr)
        {

            if (libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER) &&
                    libusb_kernel_driver_active(myHandle, interfaceIndex) != LIBUSB_SUCCESS)
            {
                // We need to remove the kernel driver to gain access to the device
                LogUsb(libusb_detach_kernel_driver(myHandle, interfaceIndex));
            }

            int currentConfig = -1;

            if (result && (LogUsb(libusb_get_configuration(myHandle, &currentConfig)) == LIBUSB_SUCCESS))
            {
                if (currentConfig != configIndex)
                {
                    LOGTRACE("Setting Configuration to " + std::to_string(configIndex));
                    result &= LogUsb(libusb_set_configuration(myHandle, configIndex)) == LIBUSB_SUCCESS;
                }
            }
            else
            {
                LOGERROR("Failed to open device");
            }

            LOGTRACE("Attempting to claim interface...");
            result &= LogUsb(libusb_claim_interface(myHandle, interfaceIndex)) == LIBUSB_SUCCESS;

            if (result)
            {
                libusb_ref_device(myDev);

                if (!eventHandler.IsRunning())
                {
                    eventHandler.Start();
                }
            }
        }
        return result;
    }

    bool Usb::Open(libusb_device  *const dev)
    {
        myDev = dev;
        return Open();
    }

    bool Usb::Close()
    {

        eventHandler.Stop();
        if (myHandle != nullptr)
        {
            for (int iface : claimedInterfaces)
            {
                LogUsb(libusb_release_interface(myHandle, iface));
            }

            libusb_close(myHandle);
        }

        return true;
    }

    void Usb::StandardReadCallback(libusb_transfer *transfer)
    {
        using namespace std;
        if (transfer != nullptr && transfer->user_data != nullptr)
        {
            Usb* child = static_cast<Usb*>(transfer->user_data);
            child->ReadCallback(transfer);
        }
    }

    void Usb::StartReadingBulk(uint8_t request)
    {
        struct libusb_transfer *transfer = libusb_alloc_transfer(0);

        libusb_fill_bulk_transfer(
            transfer, myHandle, request, readBuffer, sizeof(readBuffer),
            Usb::StandardReadCallback, this, 0);

        if (LogUsb(libusb_submit_transfer(transfer)) != LIBUSB_SUCCESS)
        {
            LOGERROR("Failed to submit usb transfer.");
        }
    }

    bool Usb::WriteBulk(DataBlock data, size_t endpoint)
    {
        int numSent = 0;
        size_t result = LogUsb(libusb_bulk_transfer(myHandle, endpoint, data.data(), static_cast<int>(data.size()), &numSent, writeTimeout));

        return result == libusb_error::LIBUSB_SUCCESS;
    }

    void Usb::Shutdown()
    {
        // This closes the default context which is reference counted
        libusb_exit(nullptr);
    }

    libusb_error Usb::LogUsb(libusb_error result)
    {
        return static_cast<libusb_error>(LogUsb(static_cast<int>(result)));
    }

    int Usb::LogUsb(int result)
    {
        if (result != LIBUSB_SUCCESS)
        {
            std::string message = "LibUsb: " + std::string(libusb_error_name(result));
// strerror is not provided by earlier versions of libusb
#if defined(libusb_strerror)
            message += " : " + std::string(libusb_strerror(static_cast<libusb_error>(result)))
#endif
                       LOGDEBUG(message);
        }
        return result;
    }

    void Usb::EventHandler::DoWork()
    {
        LOGTRACE("EventHandler Running...");
        // negative results from libusb are errors, positive are warnings
        // TODO: Still not convinced by the wording in the libusb async docs
        //      This may need to be a single thread per context, which is more difficult to manage.
        if (LogUsb(libusb_handle_events_completed(nullptr, nullptr)) < LIBUSB_SUCCESS)
        {
            LOGERROR("Libusb Events failed.");
        }
    }

    void Usb::DetectFunc(UsbList& results, bool firstOnly)
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

        // Create an object which will hopefully connect
        Usb* newDevice = new Usb();
        for (int devIndex = 0; devIndex < numDevices; devIndex++)
        {
            if (newDevice->Open(devs[devIndex]))
            {
                LOGDEBUG("Found matching vid/pid...");
                // Success, store the new device and create a blank one to work with
                results.push_back(newDevice);
                // TODO: use shared/unique pointers
                newDevice = new Usb();

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

    URI Usb::GetAddress()
    {
        URI result;
        result.SetScheme("usb");
        std::string portPath;
        if(myDev)
        {
            // get the device path
            uint8_t portNumbers[8];
            int numPortNumbers = libusb_get_port_numbers(myDev, portNumbers, sizeof (portNumbers));
            for(int index = 0; index < numPortNumbers; index++)
            {
                portPath += to_string(portNumbers[index]) + "/";
            }
            result.SetPath(portPath);
        }
        result.AddParameter("product", to_string(product));
        result.AddParameter("vendor", to_string(vendor));

        return result;
    }

}
