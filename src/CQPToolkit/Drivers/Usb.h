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
#include <functional>
#include "Algorithms/Logging/Logger.h"

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

    /// Generic communication with a USB device
    ///
    class CQPTOOLKIT_EXPORT Usb
    {
    public:
        /// Destructor
        virtual ~Usb();

        /** Perform device detection
         * @param vendorId the vendor id to find
         * @param productId the product id to find
         * @param serial the serial id to find or the first if serial is empty
         * @return A usb instance or null if no device is found
         */
        static std::unique_ptr<Usb> Detect(uint16_t vendorId, uint16_t productId, const std::string& serial = "");

        /**
         * @brief Usb
         * Construct a device based on an existing libusb object
         * @param dev The libusb object
         */
        Usb(::libusb_device *const dev);

        /**
         * @brief Open
         * Open a connection to the specified device
         * @param configIndex Activate a config index, use -1 to not change configs
         * @param interfaces The interfaces to claim, use an empty list to use the first one
         * @param detachKernelDriver detach kernel driver if one is attached to the device (it will be reatached once the device is closed)
         * @return true on success
         */
        virtual bool Open(int configIndex = -1, const std::vector<int>& interfaces = {}, bool detachKernelDriver = true);

        /// The defintion of a callback from libusb
        using CallbackFunc = void(*) (struct ::libusb_transfer *transfer);

        /** Send a block of data in bulk mode
         * @param[in] data The data to send
         * @param endpoint The endpoint id to send the data to
         * @param[in] timeout How long to wait before aborting the write, use 0 for never
         * @returns true on success
         */
        virtual bool WriteBulk(DataBlock data, unsigned char endpoint, std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

        /** Send a block of data in bulk mode
         * @param[in,out] data The storage for incomming data
         * @param[in] endPoint The endpoint to read from
         * @param[in] timeout How long to wait before aborting the read, use 0 for never
         * @returns true on success
         */
        virtual bool ReadBulk(DataBlock& data, uint8_t endPoint, std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

        /**
         * @brief StartReadingBulk start an asyncronous read, the result of which is passed to callback.
         * @details
         * At the end of the callback you must call CleanupTransfer(transfer)
         * @param endPoint  The endpoint to read from
         * @param buffer Address for incomming data
         * @param bufferLength The size of the incomming data
         * @param callback Handler for the data
         * @param userData passed to the callback in the transfer parameter
         * @return The transfer object, which can be used to canel the action with CancelTransfer()
         * If the result is null the request failed
         */
        libusb_transfer* StartReadingBulk(uint8_t endPoint, unsigned char* buffer, size_t bufferLength, CallbackFunc callback, void* userData);

        /**
         * Class to pass relevant data to the event listeners
         * @tparam C The class for the callback
         */
        template<typename C>
        struct ReadEventData {
            /// The calback signiture
            using CallbackFunc = void (C::*)(std::unique_ptr<DataBlock>);
            /// Back refernce for this
            Usb* self;
            /// callback which was passed to StartReadingBulk
            CallbackFunc callback;
            /// The instance to bind to the callback
            C* obj;
            /// The storage for the incomming data
            std::unique_ptr<DataBlock> buffer;

        };

        /**
         * @brief StartReadingBulk start an asyncronous read, the result of which is passed to callback.
         * The transfer is automatically cleared after the user function returns
         * @param endPoint  The endpoint to read from
         * @param buffer Storage for the incomming data. This must be resized to the maximum receipt size before calling,
         * it will be resized to the actual received size before passing to the callback.
         * @param func Handler for the data
         * @param obj passed to the callback in the transfer parameter
         * @return The transfer object, which can be used to canel the action with CancelTransfer()
         * If the result is null the request failed
         */
        template<typename C>
        libusb_transfer* StartReadingBulk(uint8_t endPoint, std::unique_ptr<DataBlock> buffer,  void (C::* func)(std::unique_ptr<DataBlock>) , C* obj)
        {
            ReadEventData<C>* eventData = new ReadEventData<C>();
            const auto bufferSize = buffer->size();
            // prepare the callback data
            eventData->self = this;
            eventData->buffer = move(buffer);
            eventData->obj = obj;
            eventData->callback = func;
            // attach the callback to the transfer and issue the request
            return StartReadingBulk(endPoint, &(*buffer)[0], bufferSize, &Usb::ReadCallback<C>, static_cast<void*>(eventData));
        }

        /**
         * @brief CancelTransfer
         * @param transfer The transfer object returned by StartReadingBuilk
         * @return true on success
         */
        virtual bool CancelTransfer(struct ::libusb_transfer *transfer);

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
    protected: // methods

        /**
         * @brief GetUserData returns the user data from within the structure
         * @param transfer the opaque struct
         * @return The userdata inside the struct
         */
        static void* GetUserData(struct libusb_transfer *transfer);

        /**
         * @brief GetUserData returns the buffer size from within the structure
         * @param transfer the opaque struct
         * @return The buffer size inside the struct
         */
        static std::size_t GetBufferSize(struct ::libusb_transfer *transfer);

        /**
         * Callback for marshalling data to the class which initiated the read
         * @details
         * The user data is used to pass the buffer back to the caller.
         * @tparam C The class which initiated the read
         * @param transfer The transfer details from libusb
         */
        template<typename C>
        static void ReadCallback(struct ::libusb_transfer *transfer)
        {
            using namespace std;
            // get the caller details from the user data
            auto eventData = reinterpret_cast<ReadEventData<C>*>(GetUserData(transfer));

            if(eventData && eventData->self && eventData->callback && eventData->obj)
            {
                // set the buffer size to what was actually received
                eventData->buffer->resize(GetBufferSize(transfer));
                using namespace std::placeholders;

                auto callback = bind(eventData->callback, eventData->obj, _1);
                // call the initiator with the buffer of received data
                callback(move(eventData->buffer));
                // clear up the transfer
                CleanupTransfer(transfer);
            } else {
                LOGERROR("Invalid userdata in callback");
            }
            delete eventData;
        }

        /**
         * @brief CleanupTransfer deallocates the transfer
         * @param[in,out] transfer The item to clear, this will be reset to null
         */
        static void CleanupTransfer(::libusb_transfer*& transfer);

    protected: // members

        /// The subsystem device which this is attached to
        /// @note this does not mean that the interface is connected
        ::libusb_device *myDev = nullptr;
        /// The handle to the active, connected device
        ::libusb_device_handle *myHandle = nullptr;
        /// Interfaces which are currently being used to communicate with the device
        IntList claimedInterfaces = IntList();

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
    };

} // namespace cqp
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
