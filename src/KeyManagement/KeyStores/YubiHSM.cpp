/*!
* @file
* @brief YubiHSM
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 04/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "YubiHSM.h"
#include <KeyManagement/KeyStores/PKCS11Wrapper.h>

namespace cqp
{
    namespace keygen
    {
#define YH_ALGO_OPAQUE_DATA 30

        YubiHSM::YubiHSM(const std::string& pkcsUrl, keygen::IPinCallback* callback,
                         const std::string& loadOptions):
            HSMStore(pkcsUrl, callback, loadOptions.c_str())
        {
            bytesPerKeyID = sizeof(uint16_t);
            // modify the defaults to work with the yubiHSM
            newObjDefaults->Set(CKA_CLASS, CKO_DATA);
            newObjDefaults->Set(CKA_KEY_TYPE, YH_ALGO_OPAQUE_DATA);

            findObjDefaults->Set(CKA_CLASS, CKO_DATA);
            findObjDefaults->Set(CKA_KEY_TYPE, YH_ALGO_OPAQUE_DATA);
        } // YubiHSM


        bool YubiHSM::ReserveKey(const std::string& destination, KeyID& keyId)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                AttributeList attrList {*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                // The start date cannot be used, it's value is not stored
                // ask for one more than the currently reserved list. the last one should be unreserved.
                if(session->FindObjects(attrList, reservedKeys[destination].size() + 1, found) == CKR_OK && !found.empty())
                {
                    // there's nothing on the device we can change
                    // look backwards through the list until an unreserved one is found
                    for(const auto& item :
                            {
                                found.rbegin(), found.rend()
                            })
                    {
                        KeyID tempKeyId = 0;
                        result = CheckP11(item->GetAttributeValue(CKA_ID, tempKeyId)) == CKR_OK;

                        if(result)
                        {
                            auto& myReservedKeys = reservedKeys[destination];
                            // check if its already reserved
                            result = find(myReservedKeys.begin(), myReservedKeys.end(), tempKeyId) == myReservedKeys.end();

                            if(result)
                            {
                                // we've found an unreserved key, pass the id to the caller and stop searching
                                myReservedKeys.push_back(keyId);
                                keyId = be64toh(tempKeyId);
                                break; // for
                            }
                        } // if result
                    } // for each found
                } // if found
                else
                {
                    LOGERROR("Key not found");
                } // else
            } // if InitSession
            else
            {
                LOGERROR("Not in a session");
            } // else

            return result;
        } // ReserveKey

        bool YubiHSM::RemoveKey(const std::string& destination, KeyID keyId, PSK& output)
        {
            using namespace std;

            bool result = HSMStore::RemoveKey(destination, keyId, output);
            if(result)
            {
                auto& myReservedKeys = reservedKeys[destination];
                auto reservedKeyIt = find(myReservedKeys.begin(), myReservedKeys.end(), keyId);
                if(reservedKeyIt != myReservedKeys.end())
                {
                    myReservedKeys.erase(reservedKeyIt);
                }
            }

            return result;
        }

    } // namespace keygen

} // namespace cqp
