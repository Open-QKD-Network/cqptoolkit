/*!
* @file
* @brief KeyVerifier
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 14/8/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyVerifier.h"

namespace cqp
{

    KeyVerifier::KeyVerifier()
    {

    }

    KeyVerifier::~KeyVerifier()
    {
        using namespace std;
        for(auto& element : receivedKeys)
        {
            LOGERROR("Unmatched key: " + to_string(element.first));
        }
    }

    void KeyVerifier::Receiver::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
    {
        using namespace std;
        lock_guard<mutex> lock(parent->storagelock);

        for(unsigned index = 0; index < keyData->size(); index++)
        {
            const KeyID keyid = nextId++;
            PSK& element = parent->receivedKeys[keyid];
            if(element.size() > 0)
            {
                if(element.size() == 0)
                {
                    // first reciept of this key
                    element = (*keyData)[index];
                }
                else
                {
                    // do the comparison
                    if(element != (*keyData)[index])
                    {
                        parent->Emit(keyid, element, (*keyData)[index]);
                        LOGERROR("Keys do not match: " + to_string(keyid));
                    }
                    else
                    {
                        LOGINFO("Key " + to_string(keyid) + " match");
                    }

                    // delete the stored data
                    parent->receivedKeys.erase(keyid);
                }
            }
            else
            {
                // key data not received for this id yet
                element = (*keyData)[index];
            }
        }
    }

} // namespace cqp
