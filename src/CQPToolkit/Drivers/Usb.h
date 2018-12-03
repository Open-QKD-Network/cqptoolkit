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
#include "libusb-1.0/libusb.h"

namespace cqp
{
    class Usb;

    /// A list of USB objects
    using UsbList = std::vector<Usb*>;

    /// Generic communication with a USB device
    class CQPTOOLKIT_EXPORT Usb
    {
    public:
        virtual ~Usb();
        // Device implementation

        /// @return true if the device is ready to use.
        virtual bool IsOpen() const;
        /// Create a connection to the device using the current parameters
        /// @return true if the operation was successful
        virtual bool Open();
        /// Creates a connection to the specified usb device
        /// @param[in] dev Open a connection using the previously enumerated device
        /// @return true if the operation was successful
        virtual bool Open(libusb_device *const dev);

        /**
         * @brief Open
         * Open a connection to the specified device
         * @param vendorId
         * @param productId
         * @param newConfigIndex
         * @param newInterfaceIndex
         * @return true on success
         */
        virtual bool Open(uint16_t vendorId, uint16_t productId, int newConfigIndex, int newInterfaceIndex);

        /// Disconnect from the device.
        /// @return true if disconnection completed without error
        /// @remarks The object must be return to a clean, disconnected state by this call
        ///         even if errors occur.
        virtual bool Close();

        /// Called by the subsystem when new data has arrived
        /// @param[in,out] transfer The details of the transfer under way.
        virtual void ReadCallback(struct libusb_transfer *transfer);

        /// Send a block of data in bulk mode
        /// @param[in] data The data to send
        /// @param endpoint The endpoint id to send the data to
        /// @returns true on success
        virtual bool WriteBulk(DataBlock data, std::size_t endpoint);

        /// Issue a request to the subsystem to receive data
        /// ReadCallback will be called when it arrives
        /// @param[in] request The id of the request to make
        virtual void StartReadingBulk(uint8_t request);

        /// Cleanly shutdown the subsystem
        /// @remarks This must be called before the program terminates.
        static void Shutdown();

        /// Print any error or warning in a human readable form
        /// @details Noes not produce any output on success
        /// @param[in] result The return value from a libUsb call.
        /// @return The value of result
        static libusb_error LogUsb(libusb_error result);
        /// Print any error or warning in a human readable form
        /// @details Noes not produce any output on success
        /// @param[in] result The return value from a libUsb call.
        /// @return The value of result
        static int LogUsb(int result);

        /// Perform device detection
        /// @param[out] results found devices are added to this list
        /// @param[in] firstOnly Stop after first detection
        static void DetectFunc(UsbList& results, bool firstOnly);

        /**
         * @brief GetAddress
         * @return The uri of the device
         */
        URI GetAddress();
    protected:

        /// Class to pass relevant data to the event listeners
        class EventData {};

        /// The subsystem device which this is attached to
        /// @note this does not mean that the interface is connected
        libusb_device *myDev = nullptr;
        /// The handle to the active, connected device
        libusb_device_handle *myHandle = nullptr;
        /// Interfaces which are currently being used to communicate with the device
        IntList claimedInterfaces = IntList();
        /// default constructor
        Usb();
        /// The maximum number of bytes to read in any one bulk read event
        static const int MaxBulkRead = 8192;

        /// Provides a thread for libusb to do it's event handling on.
        class EventHandler : public WorkerThread
        {
            void DoWork();
        };
        /// Provides a thread for libusb to do it's event handling on.
        EventHandler eventHandler;

        /**
         * @brief StandardReadCallback
         * Called by the subsystem when a read completes, used to marshal the call to a member function
         * @param transfer details of the transfer
         */
        static void LIBUSB_CALL StandardReadCallback(struct libusb_transfer *transfer);

        /// usb identifier
        uint16_t vendor = 0;
        /// usb identifier
        uint16_t product = 0;
        /// usb parameter
        int configIndex = 0;
        /// usb parameter
        int interfaceIndex = 0;
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
