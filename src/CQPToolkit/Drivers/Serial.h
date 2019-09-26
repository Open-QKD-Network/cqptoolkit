/*!
* @file
* @brief CQP Toolkit - Serial IO
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>                      // for operator+, string, to_string
#include <vector>                      // for allocator, vector
// for FILE_HANDLE
#include "Algorithms/Datatypes/URI.h"       // for URI
#include "Algorithms/Util/FileIO.h"
#include "CQPToolkit/cqptoolkit_export.h"

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif // defined
#if defined(_WIN32)
#elif defined(__unix)
    #include <termios.h>
#endif

/// @internal
/// Macro for turning a baud speed (e.g. 19200) into a platform defined constant so that the correct value is used:
/// On Linux, B19200 will become B19200 with a value of 0x16
/// On Windows, B19200 will become CBR_19200 with a value of 19200
/// @endinternal
#if defined(_WIN32)
    #define BAUD(x) B_ ## x = ## x
#elif defined(__unix)
    #define BAUD(x) B_ ## x = B ## x
#else
    #warning Define BAUD() for your OS
#endif

namespace cqp
{
    class Serial;
    /// A list of Serial objects
    using SerialList = std::vector<std::unique_ptr<Serial>>;

    /// Generic communication with RS232 devices
    class CQPTOOLKIT_EXPORT Serial
    {
    public:
        /// All possible speeds for the serial device given the current platform
        enum class BaudRate
        {
            // Each enumeration is referenced by prefixing a B_, eg: B_19200
            BAUD(110),
#if !defined(_WIN32)
            BAUD(134),
            BAUD(150),
#endif //!defined(_WIN32)
            BAUD(300),
            BAUD(600),
            BAUD(1200),
#if !defined(_WIN32)
            BAUD(1800),
#endif // !defined(_WIN32)
            BAUD(2400),
            BAUD(4800),
            BAUD(9600),
#if defined(_WIN32)
            BAUD(14400),
#endif //defined(_WIN32)
            BAUD(19200),
            BAUD(38400),
            BAUD(57600),
            BAUD(115200)
#if !defined(_WIN32)
            ,
            BAUD(460800),
            BAUD(500000),
            BAUD(576000),
            BAUD(921600),
            BAUD(1000000),
            BAUD(1152000),
            BAUD(1500000),
            BAUD(2000000),
            BAUD(2500000),
            BAUD(3000000),
#if !defined(__CYGWIN__)
            BAUD(3500000),
            BAUD(4000000)
#endif // !defined(__CYGWIN__)
#endif // !defined(_WIN32)
        };
        /// Create a new object
        Serial();
        /// Create a new object with initial values for the device
        /// This does not connect to the device, call Open() to connect.
        /// @param[in] portName The OS Specific name for the port
        /// @param[in] initialBaud Initialise the port with this rate
        Serial(const std::string& portName, BaudRate initialBaud = BaudRate::B_9600);
        /// default destructor
        virtual ~Serial();

        /// Perform device detection
        /// @param[out] results found devices are added to this list
        /// @param[in] firstOnly Stop after first detection
        static void Detect(SerialList& results, bool firstOnly);

        // Device implementation

        /**
         * @brief GetAddress
         * @return The uri of the device
         */
        virtual URI GetAddress()
        {
            return "serial:///" + port + "?baud=" + std::to_string(static_cast<uint32_t>(baud));
        }

        /// Determines if the device is open.
        /// @return true if the device is ready to use.
        virtual bool IsOpen() const;

        /// Create a connection to the device using the current parameters
        /// @return true if the operation was successful
        virtual bool Open();

        /// Creates a connection to the specified serial port index
        /// @return true if the operation was successful
        /// @param[in] portName The OS Specific name for the port
        /// @param[in] initialBaud Initialise the port with this rate
        virtual bool Open(const std::string& portName, BaudRate initialBaud = BaudRate::B_9600);

        /// Disconnect from the device.
        /// @return true if disconnection completed without error
        /// @remarks The object must be return to a clean, disconnected state by this call
        ///         even if errors occur.
        /// @return returns true on success
        bool Close();

        /// Send a single byte over the serial line
        /// @param data Data to send over the connection.
        /// @param num The number of characters to write
        /// @return true on success
        virtual bool Write(const char* data, uint32_t num = 1);

        /// Send a single byte over the serial line
        /// @param data Data to send over the connection.
        /// @param num The number of characters to write
        /// @return true on success
        virtual bool Write(const char& data, uint32_t num = 1);

        /// Blocking call to read a bytes from the interface
        /// @param[out] buffer The location to store the data in.
        /// @param[in,out] num Provide the length of buffer
        /// @return True on success with the number of bytes read in num
        bool Read(char& buffer, uint32_t& num);

        /// Blocking call to read a bytes from the interface
        /// @param[out] buffer The location to store the data in.
        /// @param[in,out] num Provide the length of buffer
        /// @return True on success with the number of bytes read in num
        bool Read(char* buffer, uint32_t& num);

        /**
         * @brief Flush
         */
        void Flush();
    protected:
        /// The name of the port being used
        /// @note under windows this takes the for "\\.\COM{n}"
        std::string port;
        /// The file descriptor for the open port
        FILE_HANDLE fileId = 0;
        // Serial opening parameters
        /// Read/write mode flags
        int accessMode = 0;
        /// Shared io flags
        int sharedMode = 0;
        /// Open mode flags
        int openFlags = 0;
        /// The speed of the device
        BaudRate baud = BaudRate::B_9600;

        /// Perform serial initialisation such as setting speed, parity etc
        /// @return true on success
        virtual bool Setup();
    private:
    };
}
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif
