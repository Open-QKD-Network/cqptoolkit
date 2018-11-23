/*!
* @file
* @brief HSMStore
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 25/7/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "HSMStore.h"
#include <bits/exception.h>            // for exception
#include <chrono>                      // for high_resolution_clock
#include <map>                         // for map, _Rb_tree_iterator, map<>:...
#include <memory>                      // for allocator, __shared_ptr_access
#include <utility>                     // for pair, move
#include "CQPToolkit/Util/FileIO.h"    // for ReadEntireFile
#include "CQPToolkit/Util/Util.h"      // for StrEqualI, ToDictionary, ToHex...
#include "Interfaces/IBackingStore.h"  // for IBackingStore::Keys
#include "KeyGen/PKCS11Wrapper.h"      // for AttributeList, FromPKCSString
#include "Util/Logger.h"               // for LOGERROR, LOGINFO, LOGWARN
#include "Util/URI.h"                  // for URI
#include "CQPToolkit/KeyGen/PKCS11Wrapper.h"      // for CKU_USER, CK_USER_TYPE

namespace cqp
{
    namespace keygen
    {

        bool HSMStore::InitSlot()
        {
            using namespace std;
            using namespace p11;
            bool result = false;

            if(slot)
            {
                result = true;
            }
            else
            {

                if(slotIdValid)
                {
                    slot.reset(new Slot(module, slotId));
                } // if slotid specified
                else if(module)
                {
                    // get slots with tokens inside
                    SlotList availableSlots;
                    module->GetSlotList(true, availableSlots);

                    for(auto nextSlotId : availableSlots)
                    {
                        slot.reset(new Slot(module, nextSlotId));
                        CK_TOKEN_INFO tokenInfo;
                        if(slot->GetTokenInfo(tokenInfo) == CKR_OK)
                        {
                            if((!serial.empty() &&
                                    p11::FromPKCSString(tokenInfo.serialNumber) == serial) ||
                                    (!tokenLabel.empty() &&
                                     p11::FromPKCSString(tokenInfo.label) == tokenLabel))
                            {
                                if(tokenInfo.flags & CKF_TOKEN_INITIALIZED && !(tokenInfo.flags & CKF_WRITE_PROTECTED))
                                {
                                    pinLengthLimit = tokenInfo.ulMaxPinLen;
                                    // get all the identifiers for the token
                                    serial = p11::FromPKCSString(tokenInfo.serialNumber);
                                    tokenLabel = p11::FromPKCSString(tokenInfo.label);
                                    slotId = nextSlotId;
                                    slotIdValid = true;

                                    // slot is setup and ready to go
                                    result = true;
                                    break; // for
                                }
                                else
                                {
                                    LOGERROR("Token is not viable, it must be initialised and not write protected");
                                }
                            } // if match
                        } // if slot ok
                        else
                        {
                            // no good, try the next one
                            slot.reset();
                        } // if slot !ok
                    } // for slots
                } // if module
                else
                {
                    LOGERROR("Invalid module");
                } // else
            } // if slot not set

            return result;
        }

        void HSMStore::SetID(p11::AttributeList& attrList, KeyID keyId)
        {
            using namespace std;

            // make the type as small as possible - there are issues with the way some devices
            // store numbers, some only allow 16bit ids.
            if(bytesPerKeyID == sizeof(uint8_t))
            {
                attrList.Set<uint8_t>(CKA_ID, static_cast<uint8_t>(keyId));
            }
            else if(bytesPerKeyID == sizeof(uint16_t))
            {
                attrList.Set<uint16_t>(CKA_ID, static_cast<uint16_t>(htobe16(keyId)));
            }
            else if(bytesPerKeyID == sizeof(uint32_t))
            {
                attrList.Set<uint32_t>(CKA_ID, static_cast<uint32_t>(htobe32(keyId)));
            }
            else
            {
                attrList.Set(CKA_ID, htobe64(keyId));
            }
        }

        void HSMStore::FixKeyID(KeyID& keyId)
        {
            using namespace std;

            // make the type as small as possible - there are issues with the way some devices
            // store numbers, some only allow 16bit ids.
            if(bytesPerKeyID == sizeof(uint8_t))
            {
                // do nothing
            }
            else if(bytesPerKeyID == sizeof(uint16_t))
            {
                keyId = be16toh(keyId);
            }
            else if(bytesPerKeyID == sizeof(uint32_t))
            {
                keyId = be32toh(keyId);
            }
            else
            {
                keyId = be64toh(keyId);
            }
        }

        bool HSMStore::InitSession()
        {
            using namespace std;
            using namespace p11;
            bool result = InitSlot();

            if(slot)
            {
                if(session == nullptr)
                {
                    session = Session::Create(slot, Session::DefaultFlags, this, &HSMStore::SessionEventCallback);
                }

                if(session->IsLoggedIn())
                {
                    result = true;
                }
                else
                {
                    if(pinValue.empty() && pinSource.empty() && pinCallback)
                    {
                        // ask the caller for the pin
                        pinCallback->GetHSMPin(serial, tokenLabel, login, pinValue);
                    }

                    if(!pinValue.empty())
                    {
                        result = CheckP11(session->Login(static_cast<CK_USER_TYPE>(login), pinValue)) == CKR_OK;
                    } // if pin-value
                    else if(!pinSource.empty())
                    {
                        if(fs::ReadEntireFile(pinSource, pinValue, pinLengthLimit))
                        {
                            trim(pinValue);
                            result = CheckP11(session->Login(static_cast<CK_USER_TYPE>(login), pinValue)) == CKR_OK;
                        } // if read ok
                        else
                        {
                            result = false;
                            LOGERROR("Failed to read pin from " + pinSource);
                        } // if read !ok

                    }
                    else
                    {
                        LOGERROR("No pin provided");
                    }// if pin-source
                }// if !logged in
            } // if slot ok

            return result;
        }

        unsigned int HSMStore::DeleteAllKeys()
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            unsigned int numDeleted = 0;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_DESTROYABLE, true);
                bool result = true;

                // search for the object
                while(result && session->FindObjects(attrList, 100, found) == CKR_OK && !found.empty())
                {
                    for(auto obj : found)
                    {
                        if(obj.DestroyObject() == CKR_OK)
                        {
                            numDeleted++;
                        } else {
                            result = false;
                            break;
                        }
                    } // for found
                    found.clear();
                } // while found
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return numDeleted;
        }

        bool HSMStore::RemoveKey(const std::string& destination, KeyID keyId)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                SetID(attrList, keyId);
                // search for the object
                if(session->FindObjects(attrList, 1, found) == CKR_OK && !found.empty())
                {
                    if(found[0].DestroyObject() != CKR_OK)
                    {
                        LOGERROR("Failed to destroy removed key: 0x" + ToHexString(keyId));
                        result = false;
                    }
                }
                else
                {
                    LOGERROR("Key not found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        std::set<std::string> HSMStore::GetDestinations(uint numToSearch)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            std::set<std::string> result;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                // search for the object
                if(session->FindObjects(attrList, numToSearch, found) == CKR_OK && !found.empty())
                {
                    for(auto item : found)
                    {
                        string destination;
                        item.GetAttributeValue(CKA_LABEL, destination);
                        result.insert(destination);
                    }
                }
                else
                {
                    LOGERROR("No keys found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        CK_RV HSMStore::SessionEventCallback(CK_SESSION_HANDLE, CK_NOTIFICATION, void* pApp)
        {
            try
            {

                auto self = reinterpret_cast<HSMStore*>(pApp);
                if(self)
                {
                    CK_TOKEN_INFO info;
                    if(self->slot->GetTokenInfo(info) == CKR_OK)
                    {
                        if(!(info.flags & CKF_TOKEN_PRESENT))
                        {
                            // token has been removed
                            self->session.reset();
                        }
                    } // if info ok
                } // if pApp

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return CKR_OK;
        } // SessionEventCallback

        HSMStore::HSMStore(const URI& pkcsUrl, IPinCallback* callback, const void* moduleLoadOptions) :
            pinCallback(callback),
            newObjDefaults(new p11::AttributeList()),
            findObjDefaults(new p11::AttributeList())
        {
            using namespace p11;
            using namespace std;
            std::map<string, string> keyValues;

            // collect all values into a dictionary
            pkcsUrl.ToDictionary(keyValues);
            // find the module
            auto moduleName = keyValues.find("module-name");
            if(moduleName == keyValues.end())
            {
                // it doesn't matter which key they use, it all goes to the same field
                moduleName = keyValues.find("module-path");
            } // if moduleName invalid

            if(moduleName != keyValues.end())
            {
                LOGTRACE("Loading module: " + moduleName->second);

                module = Module::Create(moduleName->second, moduleLoadOptions);
            } // if moduleName valid

            std::string loginString;
            if(pkcsUrl.GetFirstParameter("login", loginString))
            {
                if(StrEqualI(loginString, "user"))
                {
                    login = UserType::User;
                }
                else if(StrEqualI(loginString, "so"))
                {
                    login = UserType::SecurityOfficer;
                }
                else if(StrEqualI(loginString, "cs"))
                {
                    login = UserType::ContextSpecific;
                }
                else
                {
                    LOGWARN("Unknown login user type: " + loginString);
                }
            }

            if(!pkcsUrl.GetFirstParameter("source", source))
            {
                LOGWARN("Not key source defined in url.");
            }

            if(!module)
            {
                LOGERROR("Failed to load HSM module.");
            } // if module valid
            else
            {
                ::CK_INFO info {};
                CheckP11(module->GetInfo(info));

                // print library version
                LOGINFO("Loaded module \"" + FromPKCSString(info.libraryDescription) +
                        "\", By: \"" + FromPKCSString(info.manufacturerID) +
                        "\", Version: " + std::to_string( info.libraryVersion.major ) + "." +
                        std::to_string( info.libraryVersion.minor ));

                auto serialIt = keyValues.find("serial");
                auto tokenLabelIt = keyValues.find("token");
                auto slotIdIt = keyValues.find("slot-id");
                if(slotIdIt != keyValues.end())
                {
                    try
                    {
                        slotId = static_cast<uint8_t>(stoi(slotIdIt->second));
                        slotIdValid = true;
                        LOGTRACE("Using slot ID " + std::to_string(slotId));
                    }
                    catch (const exception& e)
                    {
                        LOGERROR("Invalid slot id:" + e.what());
                    }
                }
                if(serialIt != keyValues.end())
                {
                    serial = serialIt->second;
                    LOGTRACE("Using serial " + serial);
                }
                if(tokenLabelIt != keyValues.end())
                {
                    tokenLabel = URI::Decode(tokenLabelIt->second);
                    LOGTRACE("Using token " + tokenLabel);
                }

                pinValue = keyValues["pin-value"];
                pinSource = keyValues["pin-source"];

            } // if module ok

            // setup the defaults for attributes
            findObjDefaults->Set(CKA_CLASS, CKO_SECRET_KEY);
            findObjDefaults->Set(CKA_KEY_TYPE, CKK_GENERIC_SECRET);

            newObjDefaults->Set(CKA_CLASS, CKO_SECRET_KEY);
            newObjDefaults->Set(CKA_KEY_TYPE, CKK_GENERIC_SECRET);
            // This will be set to a real value once it's reserved
            newObjDefaults->Set(CKA_START_DATE, zeroStartDate);

            // accessibility...
            newObjDefaults->Set(CKA_TOKEN, true); // otherwise its a session object
            newObjDefaults->Set(CKA_EXTRACTABLE, true);
            newObjDefaults->Set(CKA_DESTROYABLE, true);
            newObjDefaults->Set(CKA_SENSITIVE, false);
            newObjDefaults->Set(CKA_PRIVATE, false);

            // can be used for...
            newObjDefaults->Set(CKA_DECRYPT, true);
            newObjDefaults->Set(CKA_ENCRYPT, true);
            newObjDefaults->Set(CKA_WRAP, true);
            newObjDefaults->Set(CKA_UNWRAP, true);


        } // HSMStore

        HSMStore::FoundTokenList HSMStore::FindTokens(const std::vector<std::string>& modules)
        {
            using namespace p11;
            using namespace std;
            FoundTokenList result;

            for(auto& mod : modules)
            {
                LOGTRACE("Trying " + mod);
                auto module = Module::Create(mod);
                if(module)
                {
                    LOGTRACE("Loaded");
                    ::CK_INFO info {};
                    CheckP11(module->GetInfo(info));
                    LOGTRACE("Getting Slot list");

                    SlotList availableSlots;
                    module->GetSlotList(true, availableSlots);
                    LOGTRACE("Found " + std::to_string(availableSlots.size()) + " slots");

                    for(auto slotId : availableSlots)
                    {
                        LOGTRACE("Slot " + std::to_string(slotId) + " found");

                        Slot slot(module, slotId);
                        LOGTRACE("Slot " + std::to_string(slotId) + " loaded");
                        CK_TOKEN_INFO tokenInfo {};
                        if(slot.GetTokenInfo(tokenInfo) == CKR_OK &&
                                (tokenInfo.flags & CKF_TOKEN_INITIALIZED) && !(tokenInfo.flags & CKF_WRITE_PROTECTED))
                        {
                            URI foundModule;
                            vector<string> pathElements;
                            LOGTRACE("Token " + FromPKCSString(tokenInfo.label) + " usable");
                            pathElements.push_back("module-name=" + mod);
                            pathElements.push_back("token=" + p11::FromPKCSString(tokenInfo.label));
                            pathElements.push_back("serial=" + p11::FromPKCSString(tokenInfo.serialNumber));
                            foundModule.SetPath(pathElements, ";", false);
                            foundModule.SetScheme("pkcs");

                            result.push_back(foundModule);

                        } // if slot ok
                    } // for slots
                } // if module
            } // for modules
            return result;
        }

        HSMStore::~HSMStore()
        {
            if(session && session->IsLoggedIn())
            {
                LOGTRACE("Logging Out");
                session->Logout();
            }
        }

        bool HSMStore::GetKey(const std::string& destination, KeyID keyId, PSK& output)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                SetID(attrList, keyId);
                // search for the object
                if(CheckP11(session->FindObjects(attrList, 1, found)) == CKR_OK && !found.empty())
                {
                    result = found[0].GetAttributeValue(CKA_VALUE, output) == CKR_OK;
                }
                else
                {
                    LOGERROR("Key not found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        bool HSMStore::FindKey(const std::string& destination, KeyID& keyId, PSK& output)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                if(keyId != 0)
                {
                    SetID(attrList, keyId);
                }
                // search for the object
                if(CheckP11(session->FindObjects(attrList, 1, found)) == CKR_OK && !found.empty())
                {
                    result = found[0].GetAttributeValue(CKA_ID, keyId) == CKR_OK;
                    FixKeyID(keyId);
                    result &= found[0].GetAttributeValue(CKA_VALUE, output) == CKR_OK;
                }
                else
                {
                    LOGERROR("Key not found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        bool HSMStore::KeyExists(const std::string& destination, KeyID keyId)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                SetID(attrList, keyId);

                // search for the object
                CheckP11(session->FindObjects(attrList, 1, found));
                result = !found.empty();
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        bool HSMStore::StoreKeys(const std::string& destination, Keys& keys)
        {
            bool result = false;
            if(InitSession())
            {
                for(auto key = keys.begin(); key != keys.end(); key++)
                {
                    LOGDEBUG("Storing key 0x" + ToHexString(key->first) + " for " + destination);
                    using namespace p11;
                    using namespace std;
                    AttributeList keyProps{*newObjDefaults};

                    keyProps.Set(CKA_LABEL, destination);
                    SetID(keyProps, key->first);

                    keyProps.Set(CKA_VALUE, key->second);
                    DataObject newKey(session);
                    result = CheckP11(newKey.CreateObject(keyProps)) == CKR_OK;
                    if(!result && key != keys.begin())
                    {
                        // something went wrong, remove the completed keys from the list
                        keys.erase(keys.begin(), key - 1);
                        break;
                    }

                } // for keys

                if(result)
                {
                    keys.clear();
                }
            } // if InitSession
            else
            {
                LOGERROR("Not in a session");
            }

            return result;
        }

        bool HSMStore::RemoveKey(const std::string& destination, KeyID keyId, PSK& output)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList{*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                SetID(attrList, keyId);
                // search for the object
                if(session->FindObjects(attrList, 1, found) == CKR_OK && !found.empty())
                {
                    result = found[0].GetAttributeValue(CKA_VALUE, output) == CKR_OK;
                    if(result)
                    {
                        if(found[0].DestroyObject() != CKR_OK)
                        {
                            LOGERROR("Failed to destroy removed key: 0x" + ToHexString(keyId));
                            result = false;
                        }
                    }
                }
                else
                {
                    LOGERROR("Key not found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        }

        bool HSMStore::RemoveKeys(const std::string& destination, Keys& keys)
        {
            bool result = true;
            for(auto key : keys)
            {
                result &= RemoveKey(destination, key.first, key.second);
            }
            return result;
        }

        bool HSMStore::ReserveKey(const std::string& destination, KeyID& keyId)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            bool result = false;

            if(InitSession())
            {
                AttributeList attrList {*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                // use the sensitive status to denote whether it's in use
                attrList.Set(CKA_START_DATE, zeroStartDate);

                if(session->FindObjects(attrList, 1, found) == CKR_OK && !found.empty())
                {
                    // set the object to sensitive to denote that it's in use.
                    AttributeList updateModifiable;
                    updateModifiable.Set(CKA_START_DATE, std::chrono::high_resolution_clock::now());
                    result = CheckP11(found[0].SetAttributeValue(updateModifiable)) == CKR_OK;
                    if(result)
                    {
                        result = CheckP11(found[0].GetAttributeValue(CKA_ID, keyId)) == CKR_OK;
                        FixKeyID(keyId);
                    }
                }
                else
                {
                    LOGERROR("Key not found");
                }
            }
            else
            {
                LOGERROR("Not in a session");
            }

            return result;
        }

        void HSMStore::GetCounts(const std::string& destination, uint64_t& availableKeys, uint64_t& remainingCapacity)
        {
            CK_TOKEN_INFO tokenInfo;
            if(slot->GetTokenInfo(tokenInfo) == CKR_OK)
            {
                remainingCapacity = tokenInfo.ulFreePrivateMemory;
            }

            using namespace p11;
            using namespace std;

            ObjectList found;
            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList {*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                // search for the objects
                session->FindObjects(attrList, std::numeric_limits<unsigned long>::max(), found);
                availableKeys = found.size();
            }
        }

        uint64_t HSMStore::GetNextKeyId(const std::string& destination)
        {
            using namespace p11;
            using namespace std;
            ObjectList found;
            uint64_t result = 1;

            if(InitSession())
            {
                // setup the search parameters
                AttributeList attrList {*findObjDefaults};
                attrList.Set(CKA_LABEL, destination);
                // search for the objects

                if(session->FindObjects(attrList, std::numeric_limits<unsigned long>::max(), found) == CKR_OK && !found.empty())
                {
                    for(auto obj : found)
                    {
                        uint64_t nextKeyId = 0;
                        if(obj.GetAttributeValue(CKA_ID, nextKeyId) == CKR_OK)
                        {
                            FixKeyID(nextKeyId);
                            if(nextKeyId > result)
                            {
                                result = nextKeyId;
                            } // if bigger
                        } // if has id
                    } // for found
                } // if found
            }
            else
            {
                LOGERROR("Not in a session");
            }
            return result;
        } // GetNextKeyId

    } // namespace keygen
} // namespace cqp

