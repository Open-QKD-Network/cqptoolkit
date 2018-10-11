/*!
* @file
* @brief CQP Toolkit - Cheap and cheerful LED driver Mk1
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22 Sep 2016
* @author David Lowndes <david.lowndes@bristol.ac.uk>
*/
#include "LEDAliceMk1.h"
#include <cstdlib>                  // for labs
#include "Datatypes/Base.h"         // for DataBlock
#include "Datatypes/Qubits.h"       // for Qubit
#include "Drivers/Serial.h"         // for Serial
#include "Drivers/Usb.h"            // for Usb
#include "Interfaces/IRandom.h"     // for IRandom
#include "QKDInterfaces/Site.pb.h"  // for Device, Side_Type, Side_Type_Alice
#include "Util/Logger.h"            // for LOGERROR
namespace cqp
{
    class ISessionController;
}

namespace cqp
{

    const std::string DriverName = "LEDAliceMk1";

    std::string LEDAliceMk1::GetDriverName() const
    {
        return DriverName;
    }

    LEDAliceMk1::LEDAliceMk1(IRandom* randomSource) : Usb(), randomness(randomSource)
    {

    }

    LEDAliceMk1::LEDAliceMk1(const std::string& portName, const std::string& description)
    {
        myPortName = portName;
    }

    bool LEDAliceMk1::Open(const std::string& portName, const std::string& description)
    {

        myPortName = portName;

        return Open();
    }


    bool LEDAliceMk1::IsOpen() const
    {
        return configPort.IsOpen() && IsOpen();
    }

    bool LEDAliceMk1::Open()
    {
        bool result = false;
        result = configPort.Open(myPortName, myBaudRate);
        if(result)
        {
            result = Usb::Open(UsbVID, UsbPID, 1, 0);
        }
        return result;
    }

    bool LEDAliceMk1::Close()
    {
        bool result = configPort.Close();
        result &= Usb::Close();
        return result;
    }

    bool LEDAliceMk1::Initialise()
    {

        //std::cout << "\tSending " << Div << ", " << Del << std::endl;
        bool result = configPort.Write(divEndpoint);
        //MySleep(1000);
        result &= configPort.Write(div10Mhz);
        //MySleep(1000);
        result &= configPort.Write(commandEnd);
        //MySleep(1000);
        result &= configPort.Write(delEndPoint);
        //MySleep(1000);
        result &= configPort.Write(del7ns);
        //MySleep(1000);
        result &= configPort.Write(commandEnd);
        //std::cout << "\tSent" << std::endl;
        return result;
    }

    remote::Device LEDAliceMk1::GetDeviceDetails()
    {
        remote::Device result;
        // TODO
        result.set_id(myPortName);
        result.set_side(remote::Side_Type::Side_Type_Alice);
        return result;
    }

    void LEDAliceMk1::Fire(unsigned long numQubits)
    {
        using namespace std;
        const unsigned long QubitsPerByte = sizeof(uint8_t)*8 / bitsPerQubit;
        const uint32_t BytesToSend = std::labs(numQubits / QubitsPerByte);

        DataBlock output;
        output.reserve(BytesToSend);

        uint8_t byteValue = 0;
        uint32_t offset = 0;

        if((numQubits % QubitsPerByte) != 0)
        {
            LOGERROR("Idiot!");
        }



        for(unsigned long count = 0; count < numQubits; count++)
        {
            Qubit bit = randomness->RandQubit();
            byteValue |= bit << offset;
            // move to the next bit
            offset = offset + bitsPerQubit;

            if(offset > 6)
            {
                // we have filled up a word, add it to the output and reset
                output.push_back(byteValue);
                byteValue = 0;
                offset = 0;
            } // if
        } // For each qubit

        WriteBulk(output, UsbEndpoint);
    }


    void LEDAliceMk1::StartFrame()
    {
    }

    void LEDAliceMk1::EndFrame()
    {
    }

    URI cqp::LEDAliceMk1::GetAddress() const
    {
        return "";
    }

    std::string LEDAliceMk1::GetDescription() const
    {
        return "";
    }

    ISessionController* LEDAliceMk1::GetSessionController()
    {
        return nullptr;
    }
}



