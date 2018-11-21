/*!
* @file
* @brief CQP Toolkit - Photon Detection
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Drivers/PhotonDetectorMk1.h"
#include <chrono>                    // for seconds
#include <thread>                    // for sleep_for
#include "CQPToolkit/Util/Logger.h"  // for LOGINFO, LOGERROR
#include "Drivers/Serial.h"          // for Serial
#include "Drivers/UsbTagger.h"       // for UsbTagger, UsbTaggerList

namespace cqp
{
    using namespace std;

    /// The name of the device
    const std::string Name = "Mk1Tagger";

    /// Wait for the time it takes the device to reset
    static void DeviceRestDelay();

    PhotonDetectorMk1::PhotonDetectorMk1(const std::string& cmdPortName) :
        serialPortName(cmdPortName)
    {
        PhotonDetectorMk1::Open();
    }

    PhotonDetectorMk1::PhotonDetectorMk1(UsbTagger* const usbDev, Serial* const serialDev)
    {
        highSpeedDev = usbDev;
        commandDev = serialDev;
    }

    static void DeviceRestDelay()
    {
        using namespace std;
        this_thread::sleep_for(chrono::seconds(1));
    }

    bool PhotonDetectorMk1::Initialise()
    {
        bool result = true;

        if (commandDev != nullptr)
        {
            const char InitSeq[] = { 'W', 'S' };
            // predefined command sequence for initialisation
            for (char cmd : InitSeq)
            {
                result &= commandDev->Write(cmd);
                DeviceRestDelay();
            }
        }

        if (highSpeedDev != nullptr)
        {
            result &= highSpeedDev->Initialise();
        }

        result &= Calibrate();

        return result;
    }

    bool PhotonDetectorMk1::Calibrate()
    {
        bool result = true;

        const char InitSeq[] = { 'A', 'B', 'C', 'D' };
        if (commandDev != nullptr)
        {
            // predefined command sequence for calibration
            for (char cmd : InitSeq)
            {
                commandDev->Write(cmd);
                DeviceRestDelay();
            }
        }
        return result;
    }

    bool PhotonDetectorMk1::BeginDetection()
    {
        bool result = false;
        if (commandDev)
        {
            result = commandDev->Write('R');
        }
        return result;
    }

    bool PhotonDetectorMk1::EndDetection()
    {
        bool result = commandDev->Write('S');
        return result;
    }

    string PhotonDetectorMk1::GetDriverName() const
    {
        return Name;
    }

    bool PhotonDetectorMk1::IsOpen() const
    {
        return (
                   commandDev != nullptr && commandDev->IsOpen() &&
                   highSpeedDev != nullptr && highSpeedDev->IsOpen()
               );
    }

    bool PhotonDetectorMk1::Open()
    {
        bool result = true;
        if (commandDev == nullptr)
        {
            commandDev = new Serial();
        }

        if (commandDev->Open(serialPortName))
        {
            LOGINFO("command port opened");
        }
        else
        {
            LOGERROR("Failed to open command port");
            result = false;
        }

        UsbTaggerList found;
        UsbTagger::DetectFunc(found, true);
        if (found.size() == 1)
        {
            LOGINFO("High speed port opened");
            highSpeedDev = found[0];
        }
        else
        {
            LOGINFO("Failed to open High speed port");
            result = false;
        }
        return result;
    }

    bool PhotonDetectorMk1::Close()
    {
        bool result = true;
        if (commandDev != nullptr)
        {
            result &= commandDev->Close();
        }

        if (highSpeedDev != nullptr)
        {
            result &= highSpeedDev->Close();
        }
        return result;
    }

    string PhotonDetectorMk1::GetDescription() const
    {
        ///@todo need to produce a description of the combination of the devices used.
        return "";
    }
    URI PhotonDetectorMk1::GetAddress() const
    {
        return "";
    }

}

