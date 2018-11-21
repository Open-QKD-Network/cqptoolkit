/*!
* @file
* @brief CQP Toolkit - Serial IO - Windows Specific
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#if defined(_WIN32)
#include "stdafx.h"
#include "CQPToolkit/Drivers/Serial.h"
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Util/Platform.h"
#include <iterator>
#include <regex>
#include <devpropdef.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <comdef.h>

DEFINE_GUID(GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR, 0x4D36E978L, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);

namespace cqp
{
    using namespace std;

    ///// Static values
    const std::string portNamePrefix = "\\\\.\\";

    bool Serial::Open(const std::string& portName, BaudRate initalBaud)
    {
        baud = initalBaud;
        if (regex_match(portName, regex("^" + portNamePrefix + ".+")))
        {
            // The supplied name already has the prefix
            port = portName;
        }
        else
        {
            port = portNamePrefix + portName;
        }

        return Open();
    }

    Serial::Serial()
    {
        accessMode = GENERIC_READ | GENERIC_WRITE;
        sharedMode = FILE_SHARE_READ;
        openFlags = FILE_ATTRIBUTE_NORMAL;
    }

    Serial::Serial(const std::string& portName, BaudRate initalBaud) :
        Serial()
    {
        baud = initalBaud;
        if (regex_match(portName, regex("^" + portNamePrefix + ".+")))
        {
            // The supplied name already has the prefix
            port = portName;
        }
        else
        {
            port = portNamePrefix + portName;
        }
    }

    Serial::~Serial()
    {
        if (fileId != nullptr && fileId != INVALID_HANDLE_VALUE)
        {
            CloseHandle(fileId);
        }
    }

    bool Serial::IsOpen() const
    {
        return (fileId != INVALID_HANDLE_VALUE);
    }

    bool Serial::Open()
    {
        using namespace std;
        bool result = true;

        fileId = CreateFileA(
                     port.c_str(),
                     accessMode,
                     sharedMode,
                     nullptr,
                     OPEN_EXISTING,
                     openFlags,
                     nullptr
                 );
        result &= IsOpen();

        if (result)
        {
            // configure the interface's speed/parity/etc.
            result = Setup();
        }
        else
        {
            LOGLASTERROR;
        }
        return result;
    }

    bool Serial::Close()
    {
        CloseHandle(fileId);
        fileId = INVALID_HANDLE_VALUE;
        return true;
    }

    bool Serial::Write(const char& data, uint32_t num)
    {
        return Write(&data, num);
    }

    bool Serial::Write(const char* data, uint32_t num)
    {
        LOGTRACE("Sending...");
        bool result = true;

        uint32_t numSent = 0;
        do
        {
            DWORD bytesWritten = 0;
            result = WriteFile(fileId, data + numSent, num - numSent, &bytesWritten, nullptr) == TRUE;

            if (!result)
            {
                // Print an error message
                LOGLASTERROR;
            }
            numSent += bytesWritten;
        }
        while(result && numSent < num);

        result &= FlushFileBuffers(fileId) == TRUE;

        return result;
    }

    bool Serial::Read(char* data, uint32_t& num)
    {
        bool result = num > 0;

        if (result)
        {
            uint32_t numRec = 0;
            DWORD bytesRead = 0;
            do
            {
                bytesRead = 0;
                result = ReadFile(fileId, data + numRec, num - numRec, &bytesRead, nullptr) == TRUE;

                if (!result)
                {
                    // Print an error message
                    LOGLASTERROR;
                }
                numRec += bytesRead;
            }
            while (result && numRec < num && bytesRead > 0);
        }

        return result;
    }

    bool Serial::Setup()
    {
        DCB portConfig = { 0 };
        /// @see https://msdn.microsoft.com/en-gb/library/windows/desktop/aa363214(v=vs.85).aspx for details
        // get the current settings and adjust them
        bool result = GetCommState(fileId, &portConfig) == TRUE;
        if (!result)
        {
            LOGLASTERROR;
        }

        portConfig.DCBlength = sizeof(portConfig);
        portConfig.BaudRate = static_cast<DWORD>(baud);
        portConfig.fBinary = TRUE;
        portConfig.fParity = FALSE;
        portConfig.ByteSize = 8;
        portConfig.Parity = NOPARITY;
        portConfig.StopBits = ONESTOPBIT;

        result = SetCommState(fileId, &portConfig) == TRUE;
        if (!result)
        {
            LOGLASTERROR;
        }

        return result;
    }

    void Serial::Detect(SerialList& results, bool firstOnly)
    {
        DWORD flags = DIGCF_PRESENT | DIGCF_DEVICEINTERFACE;
        HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR, NULL, NULL, flags);

        if (hDevInfoSet != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA devInfo;
            DWORD index = 0;
            bool result = true;

            // enumerate devices
            do
            {
                devInfo = { 0 };
                devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
                result = SetupDiEnumDeviceInfo(hDevInfoSet, index, &devInfo) == TRUE;

                DWORD portNameLength = MAX_PATH;
                DWORD dataType = REG_MULTI_SZ;
                HKEY deviceKey;

                if (result)
                {
                    deviceKey = SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
                    result = deviceKey != INVALID_HANDLE_VALUE;
                }

                if (result)
                {
                    TCHAR portName[MAX_PATH] = { 0 };
                    result &= SUCCEEDED(RegQueryValueEx(deviceKey, "PortName", NULL, &dataType, (BYTE*)portName, &portNameLength));
                    TCHAR description[MAX_PATH] = { 0 };
                    DWORD descriptionLength = MAX_PATH;
                    std::string realDesc = "";

                    if (SUCCEEDED(SetupDiGetDeviceRegistryProperty(
                                      hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dataType, (BYTE*)description, descriptionLength, &descriptionLength)))
                    {
                        realDesc = description;
                    }

                    results.push_back(new Serial(std::string(portName), realDesc));
                }

                index++;
            }
            while (result);   // enumerate devices
        }

    }

}
#endif // defined(_WIN32)
