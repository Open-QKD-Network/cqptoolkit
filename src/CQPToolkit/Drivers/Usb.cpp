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
#include "Algorithms/Logging/Logger.h"  // for LOGTRACE, LOGERROR, DefaultLogger
#include "Algorithms/Datatypes/Base.h"          // for DataBlock, IntList
#include "Algorithms/Logging/ILogger.h"      // for LogLevel, ILogger, LogLevel::Debug
           // for EnumClassHash
#include "Algorithms/Datatypes/URI.h"                // for URI
#include "libusb-1.0/libusb.h"

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
    struct UsbLevel {
        static const UsbLevelMap Lookup;
    };
    // storage for data
    const UsbLevelMap UsbLevel::Lookup = CreateUsbLevelMap();

    std::unique_ptr<Usb> Usb::Detect(uint16_t vendorId, uint16_t productId, const std::string& serial)
    {
        ::libusb_device **devs;
        // Initialize the library.
        if (libusb_init(nullptr) != LIBUSB_SUCCESS)
        {
            LOGERROR("Failed to initialise libUSB.");
        }
        // Find all connected devices.
        ssize_t numDevices = libusb_get_device_list(nullptr, &devs);
        LOGDEBUG("Found " + std::to_string(numDevices) + " usb devices.");

        // Create an object which will hopefully connect
        std::unique_ptr<Usb> result;

        for (int devIndex = 0; devIndex < numDevices; devIndex++)
        {

            /// The description of the connected device
            struct ::libusb_device_descriptor desc = libusb_device_descriptor();

            if (devs[devIndex] == nullptr || LogUsb(libusb_get_device_descriptor(devs[devIndex], &desc)) != LIBUSB_SUCCESS)
            {
                LOGERROR("failed to get device descriptor");
            }
            else
            {
                if(desc.idVendor == vendorId && desc.idProduct == productId)
                {
                    LOGTRACE("Device has " + std::to_string(desc.bNumConfigurations) + " Configurations.");
                    // opening the device will increase it's reference count, making it safe to delete the list
                    result = make_unique<Usb>(devs[devIndex]);

                    if(serial.empty() || result->GetSerialNumber() == serial)
                    {
                        result->vendor = desc.idVendor;
                        result->product = desc.idProduct;

                        LOGINFO("Using device: " + result->GetAddress().ToString());
                        // match is good

                        break; // for
                    } else {
                        // false match, drop the device
                        result.reset();
                    }
                } // if pid/vid match
            } // if device is good
        } // for each device

        libusb_free_device_list(devs, 1);
        libusb_exit(nullptr);

        return result;
    }

    Usb::Usb(::libusb_device* const dev) :
        myDev{dev}
    {
        // Initialize the library.
        // by passing null we are using the default context which is reference counted
        if (LogUsb(libusb_init(nullptr)) == LIBUSB_SUCCESS)
        {
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000106)
            libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, UsbLevel::Lookup.at(DefaultLogger().GetOutputLevel()));
#else
            libusb_set_debug(nullptr, UsbLevel::Lookup.at(DefaultLogger().GetOutputLevel()));
#endif
        }
        else
        {
            LOGERROR("Failed to initialise libUSB.");
        }
        LogUsb(libusb_open(myDev, &myHandle));
    }

    Usb::~Usb()
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
        libusb_exit(nullptr);
    }

    bool Usb::Open(int configIndex, const std::vector<int>& interfaces, bool detachKernelDriver)
    {
        bool result = true;

        if (myDev != nullptr && myHandle != nullptr)
        {

            if (!eventHandler.IsRunning())
            {
                eventHandler.Start();
            }

            if (libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER))
            //        libusb_kernel_driver_active(myHandle, interfaceIndex) != LIBUSB_SUCCESS)
            {
                // We need to remove the kernel driver to gain access to the device
                //LogUsb(libusb_detach_kernel_driver(myHandle, interfaceIndex));
                LogUsb(libusb_set_auto_detach_kernel_driver(myHandle, detachKernelDriver));
            }

            int currentConfig = -1;

            if (result && (LogUsb(libusb_get_configuration(myHandle, &currentConfig)) == LIBUSB_SUCCESS))
            {
                if (configIndex >= 0 && currentConfig != configIndex)
                {
                    LOGTRACE("Setting Configuration to " + std::to_string(configIndex));
                    result &= LogUsb(libusb_set_configuration(myHandle, configIndex)) == LIBUSB_SUCCESS;
                }
            }
            else
            {
                LOGERROR("Failed to open device");
            }

            claimedInterfaces = interfaces;

            if(interfaces.empty())
            {
                // try and find the first interface
                ::libusb_config_descriptor* config = nullptr;
                if(LogUsb(libusb_get_active_config_descriptor(myDev, &config)) == LIBUSB_SUCCESS)
                {
                    if(config->bNumInterfaces > 0)
                    {
                        claimedInterfaces.push_back(config->interface[0].altsetting->bInterfaceNumber);
                    }
                } else {
                    result = false;
                }
            } // if default interface needed

            for(auto interface : claimedInterfaces)
            {
                LOGTRACE("Attempting to claim interface " + to_string(interface) + "...");
                result &= LogUsb(libusb_claim_interface(myHandle, interface)) == LIBUSB_SUCCESS;
            } // for interfaces

        } // if device is ok
        return result;
    }

    bool Usb::WriteBulk(DataBlock data, unsigned char endpoint, std::chrono::milliseconds timeout)
    {
        int numSent = 0;
        auto result = LogUsb(libusb_bulk_transfer(myHandle, endpoint, data.data(), static_cast<int>(data.size()), &numSent, static_cast<unsigned int>(timeout.count())));

        return result == libusb_error::LIBUSB_SUCCESS;
    }

    bool Usb::ReadBulk(DataBlock& data, uint8_t endPoint, std::chrono::milliseconds timeout)
    {
        int bytesRecieved = 0;

        // create the transfer object and issue the request
        // block until data arrives
        auto result = LogUsb(libusb_bulk_transfer(
            myHandle, endPoint, data.data(), sizeof(data.size()), &bytesRecieved, static_cast<unsigned int>(timeout.count())));

        if ((result == LIBUSB_SUCCESS || result == LIBUSB_ERROR_OVERFLOW) && bytesRecieved > 0)
        {
            data.resize(static_cast<size_t>(bytesRecieved));
        } else
        {
            data.resize(0);
            LOGERROR("Failed to submit usb transfer.");
        }

        return result == LIBUSB_SUCCESS;
    }

    struct ::libusb_transfer * Usb::StartReadingBulk(uint8_t endPoint, unsigned char* buffer, size_t bufferLength,
                                                     Usb::CallbackFunc callback, void* userData,
                                                     chrono::milliseconds timeout)
    {
        struct ::libusb_transfer *transfer = libusb_alloc_transfer(0);

        if(transfer)
        {
            // create the transfer object
            ::libusb_fill_bulk_transfer(
                        transfer, myHandle, endPoint, buffer, static_cast<int>(bufferLength),
                    callback, userData, timeout.count());

            // issue the request, the callback will be called when data is ready
            if (LogUsb(libusb_submit_transfer(transfer)) != LIBUSB_SUCCESS)
            {
                LOGERROR("Failed to submit usb transfer.");
                libusb_free_transfer(transfer);
                transfer = nullptr;
            }
        } else {
            LOGERROR("Failed to allocate transfer");
        }

        return transfer;
    }

    bool Usb::CancelTransfer(libusb_transfer* transfer)
    {
        return ::libusb_cancel_transfer(transfer) == LIBUSB_SUCCESS;
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
        LOGTRACE("USB EventHandler Running...");
        // negative results from libusb are errors, positive are warnings
        // TODO: Still not convinced by the wording in the libusb async docs
        //      This may need to be a single thread per context, which is more difficult to manage.
        if (LogUsb(libusb_handle_events_completed(nullptr, nullptr)) < LIBUSB_SUCCESS)
        {
            LOGERROR("Libusb Events failed.");
        }
    }

    URI Usb::GetAddress()
    {
        URI result;
        result.SetScheme("usb");
        std::string portPath;
        if(myDev)
        {
            // get the device path
            for(auto port : GetPortNumbers())
            {
                portPath += to_string(port) + "/";
            }
            result.SetPath(portPath);
        }
        result.AddParameter("product", to_string(product));
        result.AddParameter("vendor", to_string(vendor));
        result.AddParameter("serial", GetSerialNumber());

        return result;
    }

    std::vector<uint8_t> Usb::GetPortNumbers()
    {
        std::vector<uint8_t> result(7); // because thats what the docs say
        int numPortNumbers = libusb_get_port_numbers(myDev, &result[0], static_cast<int>(result.size()));
        if(numPortNumbers != LIBUSB_ERROR_OVERFLOW)
        {
            result.resize(static_cast<size_t>(numPortNumbers));
        } else {
            result.clear();
            LOGERROR("USB bus depth too big");
        }

        return result;
    }

    std::string cqp::Usb::GetSerialNumber()
    {
        std::string serial(255u, '\0'); // max size from spec

        /// The description of the connected device
        struct ::libusb_device_descriptor desc = libusb_device_descriptor();

        if (myDev == nullptr || LogUsb(libusb_get_device_descriptor(myDev, &desc)) != LIBUSB_SUCCESS)
        {
            LOGERROR("failed to get device descriptor");
        }
        else
        {
            if(desc.iSerialNumber != 0)
            {
                auto descLength = libusb_get_string_descriptor_ascii(myHandle, desc.iSerialNumber,
                                                                     reinterpret_cast<unsigned char*>(&serial[0]),
                                                                     static_cast<int>(serial.length()));
                if(descLength > 0)
                {
                    serial.resize(static_cast<size_t>(descLength));
                } else{
                    LogUsb(descLength);
                    serial = "";
                }
            } // if has serial
        } // device ok

        return serial;
    }

    void*Usb::GetUserData(libusb_transfer* transfer)
    {
        if(transfer)
        {
            LogUsb(transfer->status);
            return transfer->user_data;
        } else {
            return nullptr;
        }
    }

    size_t Usb::GetBufferSize(libusb_transfer* transfer)
    {
        size_t result = 0;
        if(transfer && transfer->actual_length > 0)
        {
            result = static_cast<size_t>(transfer->actual_length);
        }

        return result;
    }

    void Usb::CleanupTransfer(libusb_transfer*& transfer)
    {
        if(transfer)
        {
            libusb_free_transfer(transfer);
            transfer = nullptr;
        }
    }
} // namespace cqp
