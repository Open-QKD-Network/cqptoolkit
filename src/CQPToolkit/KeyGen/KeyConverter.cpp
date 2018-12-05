/*!
* @file
* @brief CQP Toolkit - Key Converter
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27 Jul 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyConverter.h"
#include "Algorithms/Datatypes/QKDParameters.h"
#include "Algorithms/Logging/Logger.h"

namespace cqp
{

    namespace keygen
    {
        KeyConverter::KeyConverter(size_t bytesPerKey) :
            bytesInKey(bytesPerKey)
        {
            carryOverBytes.reserve(bytesInKey);
        }

        void KeyConverter::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
        {
            LOGDEBUG("Received " + std::to_string(keyData->size()) + " fragments");
            // The list of keys which will be emitted
            std::unique_ptr<KeyList> keyToEmit(new KeyList);
            DataBlock availableBytes;
            // use bytes from previous run
            availableBytes.assign(carryOverBytes.begin(), carryOverBytes.end());
            carryOverBytes.clear();

            for(const DataBlock& incomming : *keyData)
            {
                // add incomming data
                availableBytes.insert(availableBytes.end(), incomming.begin(), incomming.end());
            }

            PSK::iterator takeFrom = availableBytes.begin();

            while(availableBytes.end() - takeFrom >= static_cast<long>(bytesInKey))
            {
                PSK newKey;
                newKey.assign(takeFrom, takeFrom + static_cast<long>(bytesInKey));
                keyToEmit->push_back(newKey);
                takeFrom += static_cast<long>(bytesInKey);
            }

            availableBytes.erase(availableBytes.begin(), takeFrom);
            // put the remainder back
            carryOverBytes.assign(availableBytes.begin(), availableBytes.end());

            if(!keyToEmit->empty() && listener)
            {
                listener->OnKeyGeneration(std::move(keyToEmit));
            }
        }

    } // namespace keygen
} // namespace cqp

