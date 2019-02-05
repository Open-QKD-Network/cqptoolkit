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
#if defined(__unix)
#include <bits/stdint-uintn.h>          // for uint32_t
#include <fcntl.h>                      // for open, O_NOCTTY, O_RDWR
#include <termios.h>                    // for termios, cfsetspeed, tcflush
#include <unistd.h>                     // for close, fsync, read, write
#include <string>                       // for string, operator+, basic_string
#include <vector>                       // for vector
#include "CQPToolkit/Drivers/Serial.h"  // for Serial, SerialList, Serial::B...
#include "Algorithms/Util/FileIO.h"     // for Exists, FindGlob, IsDirectory
#include "Algorithms/Logging/Logger.h"     // for LOGTRACE


namespace cqp
{
    using namespace std;

    /// Static values
    struct SerialUnixConsts {
        static constexpr char Name[] = "Serial";
        static constexpr char SysFolder[] = "/sys/class/tty";
        static constexpr const char* deviceNames[] =
        {
            "/dev/ttyS*",
            "/dev/ttyACM*",
            "/dev/ttyUSB*"
        };
    };
    /// storage for struct values
    constexpr char SerialUnixConsts::Name[];
    /// storage for struct values
    constexpr char SerialUnixConsts::SysFolder[];
    /// storage for struct values
    constexpr const char* SerialUnixConsts::deviceNames[];

    Serial::Serial(const std::string& portName, BaudRate initialBaud):
        Serial()
    {
        port = portName;
        baud = initialBaud;
    }

    bool Serial::Open(const std::string& portName, BaudRate initialBaud)
    {
        port = portName;
        baud = initialBaud;

        return Open();
    }

    Serial::Serial()
    {
        accessMode = O_RDWR | O_NOCTTY;
    }


    Serial::~Serial()
    {
        if (fileId > 0)
        {
            close(fileId);
            fileId = 0;
        }
    }

    bool Serial::IsOpen() const
    {
        return fileId > 0;
    }

    bool Serial::Open()
    {
        using namespace std;
        fileId = open(port.c_str(), accessMode);

        bool result = IsOpen();

        result &= IsOpen();
        return result;
    }

    bool Serial::Close()
    {
        close(fileId);
        fileId = 0;
        return true;
    }

    bool Serial::Write(const char* data, uint32_t num)
    {
        uint32_t numSent = 0;
        do
        {
            numSent += write(fileId, data + numSent, num - numSent);
        }
        while(numSent < num);

        fsync(fileId);

        return true;
    }

    bool Serial::Read(char* data, uint32_t& num)
    {
        bool result = num > 0;

        if (result)
        {
            uint32_t numRec = 0;
            do
            {
                numRec += read(fileId, data + numRec, num - numRec);
            }
            while (numRec < num);
        }

        return result;
    }

    bool Serial::Setup()
    {
        struct termios serialSettings = {};
        int result = cfsetspeed(&serialSettings, static_cast<speed_t>(baud));

        // 8n1
        serialSettings.c_cflag     &=  static_cast<unsigned int>(~PARENB);
        serialSettings.c_cflag     &=  static_cast<unsigned int>(~CSTOPB);
        serialSettings.c_cflag     &=  static_cast<unsigned int>(~CSIZE);
        serialSettings.c_cflag     |=  CS8;

        tcflush(fileId, TCIFLUSH);

        return result == 0;
    }

    void Serial::Detect(SerialList& results, bool firstOnly)
    {
        using namespace std;
        LOGTRACE("Detecting serial devices");
        //struct stat buffer = {0};
        //bool sysFolderExists = stat(SysFolder, &buffer) == 0;

        if(fs::Exists(SerialUnixConsts::SysFolder) && fs::IsDirectory(SerialUnixConsts::SysFolder))
        {
            LOGTRACE("Using /sys path");
            vector<std::string> files = fs::ListChildren(SerialUnixConsts::SysFolder);

            for(const auto& child : files)
            {
                if(fs::Exists(child + "/device"))
                {
                    results.push_back(new Serial(child + "/device"));
                    if(firstOnly)
                    {
                        break; // for
                    }
                }
            }
        }
        else
        {
            LOGTRACE("Using /dev path");
            // fall back to name matching under proc
            for(const string& name : SerialUnixConsts::deviceNames)
            {

                LOGTRACE("Checking " + name);
                vector<string> files = fs::FindGlob(name);

                for(const string& foundDev : files)
                {
                    LOGTRACE("Found " + foundDev);
                    results.push_back(new Serial(foundDev));
                    if(firstOnly)
                    {
                        break; // for
                    }
                }
            }
        }
    }

}
#endif // defined(__unix)
