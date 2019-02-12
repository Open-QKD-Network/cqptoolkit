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
#pragma once
#include <bits/stdint-uintn.h>             // for uint16_t, uint8_t
#include <cstddef>                         // for size_t
#include <vector>                          // for vector
#include "Algorithms/Datatypes/Base.h"     // for IntList, DataBlock
#include "Algorithms/Datatypes/URI.h"           // for URI
#include "Algorithms/Util/WorkerThread.h"  // for WorkerThread

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
    #pragma warning(disable:4200)
#endif

// declarations for libusb
struct libusb_device;
struct libusb_device_handle;
struct libusb_transfer;

namespace cqp
{
    class Usb;

    /// A list of USB objects
    using UsbList = std::vector<Usb*>;

    /// Generic communication with a USB device
    ///
    class CQPTOOLKIT_EXPORT Usb
    {
    public:
        virtual ~Usb();
        // Device implementation

        /// Perform device detection
        /// @param[out] results found devices are added to this list
        /// @param[in] firstOnly Stop after first detection
        static std::unique_ptr<Usb> Detect(uint16_t vendorId, uint16_t productId, const std::string& serial = "");

        /// Creates a connection to the specified usb device
        /// @param[in] dev Open a connection using the previously enumerated device
        Usb(::libusb_device *const dev);

        /**
         * @brief Open
         * Open a connection to the specified device
         * @param newConfigIndex Activate a config index, use -1 to not change configs
         * @param newInterfaceIndex The interface to claim, use -1 to use the first one
         * @param detachKernelDriver detach kernel driver if one is attached to the device (it will be reatached once the device is closed)
         * @return true on success
         */
        virtual bool Open(int configIndex = -1, const std::vector<int>& interfaces = {}, bool detachKernelDriver = true);

        /// Called by the subsystem when new data has arrived
        /// @param[in,out] transfer The details of the transfer under way.
        using ReadCallback = void(*) (struct ::libusb_transfer *transfer);

        /// Send a block of data in bulk mode
        /// @param[in] data The data to send
        /// @param endpoint The endpoint id to send the data to
        /// @returns true on success
        virtual bool WriteBulk(DataBlock data, unsigned char endpoint);

        /// Issue a request to the subsystem to receive data
        /// ReadCallback will be called when it arrives
        /// @param[in] request The id of the request to make
        virtual void StartReadingBulk(ReadCallback callback, uint8_t request, void* userData = nullptr);

        /// Print any error or warning in a human readable form
        /// @details Noes not produce any output on success
        /// @param[in] result The return value from a libUsb call.
        /// @return The value of result
        static int LogUsb(int result);

        /**
         * @brief GetAddress
         * @return The uri of the device
         */
        URI GetAddress();

        /**
         * @brief GetPortNumbers
         * @return the path to the device in port numbers
         */
        std::vector<uint8_t> GetPortNumbers();

        /**
         * @brief GetSerialNumber
         * @return The unique ID for the device - this may not be supported by the device
         */
        std::string GetSerialNumber();
    protected:

        /// Class to pass relevant data to the event listeners
        class EventData {};

        /// The subsystem device which this is attached to
        /// @note this does not mean that the interface is connected
        ::libusb_device *myDev = nullptr;
        /// The handle to the active, connected device
        ::libusb_device_handle *myHandle = nullptr;
        /// Interfaces which are currently being used to communicate with the device
        IntList claimedInterfaces = IntList();

        /// The maximum number of bytes to read in any one bulk read event
        static const int MaxBulkRead = 8192;

        /// Provides a thread for libusb to do it's event handling on.
        class EventHandler : public WorkerThread
        {
            void DoWork();
        };
        /// Provides a thread for libusb to do it's event handling on.
        EventHandler eventHandler;

        /// usb identifier
        uint16_t vendor = 0;
        /// usb identifier
        uint16_t product = 0;
        /// usb parameter
        unsigned int writeTimeout = 0;
    private:
        /// buffer for incoming data
        uint8_t readBuffer[MaxBulkRead] = { 0 };

    };

}
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
