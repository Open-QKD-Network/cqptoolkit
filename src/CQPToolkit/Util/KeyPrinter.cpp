/*!
* @file
* @brief CQP Toolkit - Random Number Generator
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyPrinter.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include <iomanip>

namespace cqp
{
    std::mutex KeyPrinter::outputGuard;

    KeyPrinter::KeyPrinter(const std::string& prefix): outputPrefix(prefix)
    {

    }

    void KeyPrinter::SetOutputPrefix(const std::string& newPrefix)
    {
        std::lock_guard<std::mutex> lock(outputGuard);
        outputPrefix = newPrefix;
    }

    void KeyPrinter::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
    {
        using namespace std;

        stringstream message;
        for(size_t index = 0; index < keyData->size(); index++)
        {
            // build the message as hexadecimal uppercase numbers
            message << outputPrefix << "Key: " << "0x";
            for(auto byte : (*keyData)[index])
            {
                message << setw(sizeof(byte) * 2) << setfill('0') << uppercase << hex << +byte;
            }
        }

        {
            // Prevent multiple threads from writing to the output at the same time.
            std::lock_guard<std::mutex> lock(outputGuard);
            LOGINFO(message.str());
        }
    }
}
