/*!
* @file
* @brief DeviceIO
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 7/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <iostream>
#include "CQPToolkit/Util/Platform.h"
#include <chrono>
#include <condition_variable>
#include <cryptopp/filters.h>

namespace cqp
{
    namespace tunnels
    {
        /**
         * @brief The DeviceIO class
         * Common class for devices which can be used for general data io
         */
        class CQPTOOLKIT_EXPORT DeviceIO : public CryptoPP::Bufferless<CryptoPP::Sink>
        {
        public:
            /// Destructor
            virtual ~DeviceIO() override {}

            /**
             * @brief WaitUntilReady
             * @param timeout Duration to wait before failing
             * @return true if the device is ready
             */
            bool WaitUntilReady(std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)) noexcept;

            /**
             * @brief Read
             * Read bytes from the device
             * @param[in,out] data Destination for the read bytes
             * @param[in] length The size of the data buffer
             * @param[out] bytesReceived The number of bytes actually read
             * @return true on success
             */
            virtual bool Read(void* data, size_t length, size_t& bytesReceived) = 0;
            /**
             * @brief Write Send bytes to the device
             * @param data The data to write
             * @param length The size of the buffer
             * @return true on success
             */
            virtual bool Write(const void* data, size_t length) = 0;

            ///@{
            /// BufferedTransformation interface

            /**
             * Calls write to send bytes to the device
             * @brief Input a byte array for processing
             * @param inString the byte array to process
             * @param length the size of the string, in bytes
             * @param messageEnd means how many filters to signal MessageEnd() to, including this one
             * @param blocking specifies whether the object should block when processing input
             * @returns the number of bytes that remain in the block (i.e., bytes not processed
             *
             */
            size_t Put2(const unsigned char* inString, size_t length, int messageEnd, bool blocking) override;

            ///@}
        protected:

            /// is the socket ready
            bool ready = false;
            /// for detecting changes to data
            std::condition_variable readyCv;
            /// protection for changes
            std::mutex readyMutex;

        };
    } // namespace tunnels
} // namespace cqp
