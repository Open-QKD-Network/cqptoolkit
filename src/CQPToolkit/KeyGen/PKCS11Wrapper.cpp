/*!
* @file
* @brief PKCS11Wrapper
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/7/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "PKCS11Wrapper.h"
#include <pkcs11/pkcs11.h>
#include <map>
#if defined(__unix__)
    #include <dlfcn.h>
#endif
#include <algorithm>
#include <time.h>
#include <cstring>

namespace cqp
{
    namespace p11
    {

        const std::map<CK_RV, std::string> ErrorCodes =
        {
            { CKR_OK, "OK"                                },
            { CKR_CANCEL, "CANCEL"                            },
            { CKR_HOST_MEMORY, "HOST_MEMORY"                       },
            { CKR_SLOT_ID_INVALID, "SLOT_ID_INVALID"                   },
            { CKR_GENERAL_ERROR, "GENERAL_ERROR"                     },
            { CKR_FUNCTION_FAILED, "FUNCTION_FAILED"                   },
            { CKR_ARGUMENTS_BAD, "ARGUMENTS_BAD"                     },
            { CKR_NO_EVENT, "NO_EVENT"                          },
            { CKR_NEED_TO_CREATE_THREADS, "NEED_TO_CREATE_THREADS"            },
            { CKR_CANT_LOCK, "CANT_LOCK"                         },
            { CKR_ATTRIBUTE_READ_ONLY, "ATTRIBUTE_READ_ONLY"               },
            { CKR_ATTRIBUTE_SENSITIVE, "ATTRIBUTE_SENSITIVE"               },
            { CKR_ATTRIBUTE_TYPE_INVALID, "ATTRIBUTE_TYPE_INVALID"            },
            { CKR_ATTRIBUTE_VALUE_INVALID, "ATTRIBUTE_VALUE_INVALID"           },
            { CKR_ACTION_PROHIBITED, "ACTION_PROHIBITED"                 },
            { CKR_DATA_INVALID, "DATA_INVALID"                      },
            { CKR_DATA_LEN_RANGE, "DATA_LEN_RANGE"                    },
            { CKR_DEVICE_ERROR, "DEVICE_ERROR"                      },
            { CKR_DEVICE_MEMORY, "DEVICE_MEMORY"                     },
            { CKR_DEVICE_REMOVED, "DEVICE_REMOVED"                    },
            { CKR_ENCRYPTED_DATA_INVALID, "ENCRYPTED_DATA_INVALID"            },
            { CKR_ENCRYPTED_DATA_LEN_RANGE, "ENCRYPTED_DATA_LEN_RANGE"          },
            { CKR_FUNCTION_CANCELED, "FUNCTION_CANCELED"                 },
            { CKR_FUNCTION_NOT_PARALLEL, "FUNCTION_NOT_PARALLEL"             },
            { CKR_FUNCTION_NOT_SUPPORTED, "FUNCTION_NOT_SUPPORTED"            },
            { CKR_KEY_HANDLE_INVALID, "KEY_HANDLE_INVALID"                },
            { CKR_KEY_SIZE_RANGE, "KEY_SIZE_RANGE"                    },
            { CKR_KEY_TYPE_INCONSISTENT, "KEY_TYPE_INCONSISTENT"             },
            { CKR_KEY_NOT_NEEDED, "KEY_NOT_NEEDED"                    },
            { CKR_KEY_CHANGED, "KEY_CHANGED"                       },
            { CKR_KEY_NEEDED, "KEY_NEEDED"                        },
            { CKR_KEY_INDIGESTIBLE, "KEY_INDIGESTIBLE"                  },
            { CKR_KEY_FUNCTION_NOT_PERMITTED, "KEY_FUNCTION_NOT_PERMITTED"        },
            { CKR_KEY_NOT_WRAPPABLE, "KEY_NOT_WRAPPABLE"                 },
            { CKR_KEY_UNEXTRACTABLE, "KEY_UNEXTRACTABLE"                 },
            { CKR_MECHANISM_INVALID, "MECHANISM_INVALID"                 },
            { CKR_MECHANISM_PARAM_INVALID, "MECHANISM_PARAM_INVALID"           },
            { CKR_OBJECT_HANDLE_INVALID, "OBJECT_HANDLE_INVALID"             },
            { CKR_OPERATION_ACTIVE, "OPERATION_ACTIVE"                  },
            { CKR_OPERATION_NOT_INITIALIZED, "OPERATION_NOT_INITIALIZED"         },
            { CKR_PIN_INCORRECT, "PIN_INCORRECT"                     },
            { CKR_PIN_INVALID, "PIN_INVALID"                       },
            { CKR_PIN_LEN_RANGE, "PIN_LEN_RANGE"                     },
            { CKR_PIN_EXPIRED, "PIN_EXPIRED"                       },
            { CKR_PIN_LOCKED, "PIN_LOCKED"                        },
            { CKR_SESSION_CLOSED, "SESSION_CLOSED"                    },
            { CKR_SESSION_COUNT, "SESSION_COUNT"                     },
            { CKR_SESSION_HANDLE_INVALID, "SESSION_HANDLE_INVALID"            },
            { CKR_SESSION_PARALLEL_NOT_SUPPORTED, "SESSION_PARALLEL_NOT_SUPPORTED"    },
            { CKR_SESSION_READ_ONLY, "SESSION_READ_ONLY"                 },
            { CKR_SESSION_EXISTS, "SESSION_EXISTS"                    },
            { CKR_SESSION_READ_ONLY_EXISTS, "SESSION_READ_ONLY_EXISTS"          },
            { CKR_SESSION_READ_WRITE_SO_EXISTS, "SESSION_READ_WRITE_SO_EXISTS"      },
            { CKR_SIGNATURE_INVALID, "SIGNATURE_INVALID"                 },
            { CKR_SIGNATURE_LEN_RANGE, "SIGNATURE_LEN_RANGE"               },
            { CKR_TEMPLATE_INCOMPLETE, "TEMPLATE_INCOMPLETE"               },
            { CKR_TEMPLATE_INCONSISTENT, "TEMPLATE_INCONSISTENT"             },
            { CKR_TOKEN_NOT_PRESENT, "TOKEN_NOT_PRESENT"                 },
            { CKR_TOKEN_NOT_RECOGNIZED, "TOKEN_NOT_RECOGNIZED"              },
            { CKR_TOKEN_WRITE_PROTECTED, "TOKEN_WRITE_PROTECTED"             },
            { CKR_UNWRAPPING_KEY_HANDLE_INVALID, "UNWRAPPING_KEY_HANDLE_INVALID"     },
            { CKR_UNWRAPPING_KEY_SIZE_RANGE, "UNWRAPPING_KEY_SIZE_RANGE"         },
            { CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT, "UNWRAPPING_KEY_TYPE_INCONSISTENT"  },
            { CKR_USER_ALREADY_LOGGED_IN, "USER_ALREADY_LOGGED_IN"            },
            { CKR_USER_NOT_LOGGED_IN, "USER_NOT_LOGGED_IN"                },
            { CKR_USER_PIN_NOT_INITIALIZED, "USER_PIN_NOT_INITIALIZED"          },
            { CKR_USER_TYPE_INVALID, "USER_TYPE_INVALID"                 },
            { CKR_USER_ANOTHER_ALREADY_LOGGED_IN, "USER_ANOTHER_ALREADY_LOGGED_IN"    },
            { CKR_USER_TOO_MANY_TYPES, "USER_TOO_MANY_TYPES"               },
            { CKR_WRAPPED_KEY_INVALID, "WRAPPED_KEY_INVALID"               },
            { CKR_WRAPPED_KEY_LEN_RANGE, "WRAPPED_KEY_LEN_RANGE"             },
            { CKR_WRAPPING_KEY_HANDLE_INVALID, "WRAPPING_KEY_HANDLE_INVALID"       },
            { CKR_WRAPPING_KEY_SIZE_RANGE, "WRAPPING_KEY_SIZE_RANGE"           },
            { CKR_WRAPPING_KEY_TYPE_INCONSISTENT, "WRAPPING_KEY_TYPE_INCONSISTENT"    },
            { CKR_RANDOM_SEED_NOT_SUPPORTED, "RANDOM_SEED_NOT_SUPPORTED"         },
            { CKR_RANDOM_NO_RNG, "RANDOM_NO_RNG"                     },
            { CKR_DOMAIN_PARAMS_INVALID, "DOMAIN_PARAMS_INVALID"             },
            { CKR_CURVE_NOT_SUPPORTED, "CURVE_NOT_SUPPORTED"               },
            { CKR_BUFFER_TOO_SMALL, "BUFFER_TOO_SMALL"                  },
            { CKR_SAVED_STATE_INVALID, "SAVED_STATE_INVALID"               },
            { CKR_INFORMATION_SENSITIVE, "INFORMATION_SENSITIVE"             },
            { CKR_STATE_UNSAVEABLE, "STATE_UNSAVEABLE"                  },
            { CKR_CRYPTOKI_NOT_INITIALIZED, "CRYPTOKI_NOT_INITIALIZED"          },
            { CKR_CRYPTOKI_ALREADY_INITIALIZED, "CRYPTOKI_ALREADY_INITIALIZED"      },
            { CKR_MUTEX_BAD, "MUTEX_BAD"                         },
            { CKR_MUTEX_NOT_LOCKED, "MUTEX_NOT_LOCKED"                  },
            { CKR_NEW_PIN_MODE, "NEW_PIN_MODE"                      },
            { CKR_NEXT_OTP, "NEXT_OTP"                          },
            { CKR_EXCEEDED_MAX_ITERATIONS, "EXCEEDED_MAX_ITERATIONS"           },
            { CKR_FIPS_SELF_TEST_FAILED, "FIPS_SELF_TEST_FAILED"             },
            { CKR_LIBRARY_LOAD_FAILED, "LIBRARY_LOAD_FAILED"               },
            { CKR_PIN_TOO_WEAK, "PIN_TOO_WEAK"                      },
            { CKR_PUBLIC_KEY_INVALID, "PUBLIC_KEY_INVALID"                },
            { CKR_FUNCTION_REJECTED, "FUNCTION_REJECTED"                 },
            { CKR_VENDOR_DEFINED, "VENDOR_DEFINED"                    }
        };

        CK_RV CheckP11(CK_RV retVal)
        {
            if(retVal != CKR_OK)
            {
                LOGERROR("Command failed with: " + ErrorCodes.at(retVal));
            }
            return retVal;
        }

        std::map<std::string, std::weak_ptr<Module>> Module::loadedModules;

        std::shared_ptr<Module> Module::Create(const std::string& libName, const void* reserved)
        {
            std::shared_ptr<Module> result;
            if(!loadedModules[libName].expired())
            {
                result = loadedModules[libName].lock();
            } else {
                result.reset(new Module());

                result->initArgs.flags = CKF_OS_LOCKING_OK;
                result->initArgs.pReserved = const_cast<void*>(reserved);

                CK_C_GetFunctionList pC_GetFunctionList = nullptr;
    #if defined(__unix__)
                result->libHandle = ::dlopen(libName.c_str(), RTLD_LAZY);
    #elif defined(WIN32)
                TODO
    #endif

                if(result->libHandle)
                {

    #if defined(__unix__)
                    pC_GetFunctionList = reinterpret_cast<CK_C_GetFunctionList>(::dlsym(result->libHandle, "C_GetFunctionList"));
    #elif defined(WIN32)
                    TODO
    #endif
                    if(pC_GetFunctionList && pC_GetFunctionList(&result->functions) == CKR_OK)
                    {
                        if(CheckP11(result->functions->C_Initialize(&result->initArgs)) != CKR_OK)
                        {
                            LOGERROR("Failed to initialise module");
                            result.reset();
                        } else {
                            loadedModules[libName] = std::weak_ptr<Module>(result);
                        }
                    }
                    else
                    {
                        LOGERROR("Failed to get funtion list");
                        result.reset();
                    }
                }
                else
                {
                    LOGERROR("Failed to load library " + libName + ": " + dlerror());
                    result.reset();
                }

            }

            return result;
        }

        Module::~Module()
        {
            if(functions)
            {
                if(functions->C_Finalize)
                {
                    functions->C_Finalize(nullptr);
                }
            }

            if(libHandle)
            {
#if defined(__unix__)
                ::dlclose(libHandle);
                libHandle = nullptr;
#elif defined(WIN32)
                TODO
#endif
            }
        }

        CK_RV Module::GetInfo(CK_INFO& info) const
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetInfo)
            {
                result = functions->C_GetInfo(&info);
            }
            return result;
        }

        CK_RV Module::GetSlotList(bool tokenPresent, SlotList& slots)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetSlotList)
            {
                unsigned long numSlots = 0;
                slots.clear();
                // find out how many slots are there
                result = functions->C_GetSlotList(tokenPresent, nullptr, &numSlots);
                if(result == CKR_OK && numSlots > 0)
                {
                    // reserve space
                    slots.resize(numSlots);
                    // retrieve the slots
                    result = functions->C_GetSlotList(tokenPresent, slots.data(), &numSlots);
                }
            }
            return result;
        }

        Slot::Slot(std::shared_ptr<Module> module, CK_SLOT_ID slotId) :
            myModule(module),
            id(slotId),
            functions(module->P11Lib())
        {

        }

        Slot::~Slot()
        {
            if(functions && functions->C_CloseAllSessions)
            {
                functions->C_CloseAllSessions(id);
            }
        }

        CK_RV Slot::InitToken(const std::string& pin, const std::string& label)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_InitToken)
            {
                std::string paddedLabel = label;
                paddedLabel.resize(LabelSize, ' ');
                result = functions->C_InitToken(
                             id,
                             reinterpret_cast<CK_UTF8CHAR*>(const_cast<char*>(pin.data())),
                             pin.size(),
                             reinterpret_cast<CK_UTF8CHAR*>(const_cast<char*>(paddedLabel.c_str()))
                         );
            }
            return result;
        }

        CK_RV Slot::GetMechanismList(MechanismList& mechanismList)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetMechanismList)
            {
                unsigned long numInList = 0;
                mechanismList.clear();
                result = functions->C_GetMechanismList(id, nullptr, &numInList);
                if(result == CKR_OK && numInList > 0)
                {
                    mechanismList.resize(numInList);
                    result = functions->C_GetMechanismList(id, mechanismList.data(), &numInList);
                }
            }
            return result;
        }

        CK_RV Slot::GetMechanismInfo(CK_MECHANISM_TYPE mechType, CK_MECHANISM_INFO& info)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetMechanismInfo)
            {
                result = functions->C_GetMechanismInfo(id, mechType, &info);
            }
            return result;
        }

        CK_RV Slot::GetTokenInfo(CK_TOKEN_INFO& tokenInfo)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetTokenInfo)
            {
                result = functions->C_GetTokenInfo(id, &tokenInfo);
            }
            return result;
        }

        Session::Session(std::shared_ptr<Slot> slot, CK_FLAGS flags, void* callbackData, const CK_NOTIFY callback):
            mySlot(slot),
            functions(slot->GetModule()->P11Lib())
        {
            if(functions && functions->C_OpenSession)
            {
                CheckP11(functions->C_OpenSession(mySlot->GetID(), flags, callbackData, callback, &handle));
            }
            else
            {
                LOGERROR("Failed to open session, function not available");
            }
        }

        Session::~Session()
        {
            CloseSession();
        }

        CK_RV Session::Login(CK_USER_TYPE userType, const std::string& pin)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_Login)
            {
                result = functions->C_Login(handle, userType,
                                            reinterpret_cast<CK_UTF8CHAR*>(const_cast<char*>(pin.data())),
                                            pin.size()
                                           );
                loggedIn = result == CKR_OK;
            }
            return result;
        }

        CK_RV Session::Logout()
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_Logout)
            {
                result = functions->C_Logout(handle);
                loggedIn = false;
            }
            return result;
        }

        CK_RV Session::GetSessionInfo(CK_SESSION_INFO& info)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetSessionInfo)
            {
                result = functions->C_GetSessionInfo(handle, &info);
            }
            return result;
        }

        CK_RV Session::CloseSession()
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_CloseSession)
            {
                result = functions->C_CloseSession(handle);
            }
            return result;
        }

        CK_RV Session::FindObjects(const AttributeList& searchParams, unsigned long maxResults, ObjectList& results)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_FindObjectsInit && functions->C_FindObjects && functions->C_FindObjectsFinal)
            {
                // start the search by specifying the parameters

                result = functions->C_FindObjectsInit(handle, searchParams.GetAttributes(), searchParams.GetCount());
                if(result == CKR_OK)
                {
                    // cap the amount we reserve each time we get more results.
                    const unsigned long numToGetEachTime = std::min(100ul, maxResults);
                    unsigned long numThisTime = 0;
                    unsigned long numSoFar = 0;
                    std::vector<CK_OBJECT_HANDLE> objHandles;

                    do
                    {
                        objHandles.resize(objHandles.size() + numToGetEachTime);
                        // the interface doesn't allow for asking how many there are,
                        // just have to keep getting until theres non left
                        result = CheckP11(functions->C_FindObjects(handle, &objHandles[numSoFar], numToGetEachTime, &numThisTime));
                        numSoFar += numThisTime;
                        objHandles.resize(numSoFar);
                    } // stop when theres an error, we've got our max results or theres no more to get
                    while(result == CKR_OK && numSoFar < maxResults && numThisTime > 0);

                    if(result == CKR_OK && objHandles.size() > 0)
                    {
                        // create the objects for the results
                        results.reserve(objHandles.size());
                        for(auto objHandle : objHandles)
                        {
                            results.push_back(DataObject(shared_from_this(), objHandle));
                        }
                    }

                    CheckP11(functions->C_FindObjectsFinal(handle));
                }
            }
            return result;
        }

        CK_RV Session::WrapKey(CK_MECHANISM_PTR mechanism, const DataObject &wrappingKey, const DataObject &key, std::vector<uint8_t> &wrappedKey)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_WrapKey)
            {
                ulong finalWrappedKeySize = 0;
                result = functions->C_WrapKey(handle, mechanism, wrappingKey.Handle(), key.Handle(), nullptr, &finalWrappedKeySize);

                if(result == CKR_OK)
                {
                    wrappedKey.resize(finalWrappedKeySize);
                    result = functions->C_WrapKey(handle, mechanism, wrappingKey.Handle(), key.Handle(), wrappedKey.data(), &finalWrappedKeySize);
                }

            }

            return result;
        }

        CK_RV Session::UnwrapKey(CK_MECHANISM_PTR mechanism, const DataObject &unwrappingKey, const std::vector<uint8_t> &wrappedkey, const AttributeList &keyTemplate, DataObject &key)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_UnwrapKey)
            {
                CK_OBJECT_HANDLE keyHandle = 0;
                functions->C_UnwrapKey(handle, mechanism, unwrappingKey.Handle(),
                                       const_cast<unsigned char*>(wrappedkey.data()), wrappedkey.size(),
                                       keyTemplate.GetAttributes(), keyTemplate.GetCount(),
                                       &keyHandle);
                key.SetHandle(keyHandle);
            }
            return result;
        }

        void AttributeList::Set(CK_ATTRIBUTE_TYPE type)
        {
            // allocate/find the storage slot
            auto& currentStorage = valueStorage[type];
            if(currentStorage.attribute == nullptr)
            {
                attributes.push_back(CK_ATTRIBUTE({}));
                currentStorage.attribute = &attributes.back();
            }
            // setup the attribute fields
            currentStorage.attribute->type = type;
            currentStorage.value.clear();
        }

        void AttributeList::Set(CK_ATTRIBUTE_TYPE type, const std::string& value)
        {

            // prepare the storage
            Set(type);
            // allocate/find the storage slot
            auto& currentStorage = valueStorage[type];
            // copy the data into the storage
            currentStorage.value.assign(value.begin(), value.end());
            // setup the attribute fields, this will exists because of caling Set(type) above
            currentStorage.attribute->type = type;
            currentStorage.attribute->pValue = currentStorage.value.data();
            currentStorage.attribute->ulValueLen = currentStorage.value.size();
        }

        void AttributeList::Set(CK_ATTRIBUTE_TYPE type, std::chrono::system_clock::time_point time)
        {
            using namespace std;
            using namespace std::chrono;
            ::CK_DATE date {};

            // jump through loads of hoops to get the date as seperate strings
            const auto in_time_t = std::chrono::system_clock::to_time_t(time);
            const auto whateverthefputwants = std::localtime(&in_time_t);

            stringstream tempStr;
            tempStr << std::put_time(whateverthefputwants, "%Y");
            strncpy(reinterpret_cast<char*>(date.year), tempStr.str().c_str(), sizeof(date.year));
            tempStr.clear();
            tempStr.str(""); // yes you really need both - DSTM

            tempStr << std::put_time(whateverthefputwants, "%m");
            strncpy(reinterpret_cast<char*>(date.month), tempStr.str().c_str(), sizeof(date.month));
            tempStr.clear();
            tempStr.str(""); // yes you really need both - DSTM

            tempStr << std::put_time(whateverthefputwants, "%d");
            strncpy(reinterpret_cast<char*>(date.day), tempStr.str().c_str(), sizeof(date.day));
            tempStr.clear();
            tempStr.str(""); // yes you really need both - DSTM

            // prepare the storage
            Set(type);
            // allocate/find the storage slot
            auto& currentStorage = valueStorage[type];
            // copy the data into the storage
            auto* valuePtr = reinterpret_cast<uint8_t*>(&date);
            currentStorage.value.assign(valuePtr, valuePtr + sizeof (date));
            // setup the attribute fields, this will exists because of caling Set(type) above
            currentStorage.attribute->type = type;
            currentStorage.attribute->pValue = currentStorage.value.data();
            currentStorage.attribute->ulValueLen = currentStorage.value.size();
        }

        bool AttributeList::Get(CK_ATTRIBUTE_TYPE type, std::vector<uint8_t>& output)
        {
            bool result = false;
            try
            {
                auto it = valueStorage.find(type);
                if(it != valueStorage.end() && it->second.attribute)
                {
                    output.clear();
                    output.reserve(it->second.attribute->ulValueLen);
                    auto* valuePtr = reinterpret_cast<uint8_t*>(it->second.attribute->pValue);
                    output.assign(valuePtr, valuePtr + it->second.attribute->ulValueLen);
                    result = true;
                }
            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return result;
        }

        bool AttributeList::Get(CK_ATTRIBUTE_TYPE type, PSK& output)
        {
            bool result = false;
            try
            {
                auto it = valueStorage.find(type);
                if(it != valueStorage.end() && it->second.attribute)
                {
                    output.clear();
                    output.reserve(it->second.attribute->ulValueLen);
                    auto* valuePtr = reinterpret_cast<uint8_t*>(it->second.attribute->pValue);
                    output.assign(valuePtr, valuePtr + it->second.attribute->ulValueLen);
                    result = true;
                }
            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }
            return result;
        }

        void AttributeList::ReserveStorage()
        {
            for(auto& element : valueStorage)
            {
                element.second.value.resize(element.second.attribute->ulValueLen);
                element.second.attribute->pValue = &element.second.value[0];
            }
        }

        DataObject::DataObject(std::shared_ptr<Session> session) :
            mySession(session),
            functions(session->GetSlot()->GetModule()->P11Lib())
        {

        }

        DataObject::DataObject(std::shared_ptr<Session> session, CK_OBJECT_HANDLE handle) :
            mySession(session),
            handle(handle),
            functions(session->GetSlot()->GetModule()->P11Lib())
        {

        }

        CK_RV DataObject::CreateObject(const AttributeList& values)
        {

            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_CreateObject)
            {
                result = functions->C_CreateObject(mySession->GetSessionHandle(),
                                                   values.GetAttributes(), values.GetCount(), &handle);
            }
            return result;
        }

        CK_RV DataObject::DestroyObject()
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_DestroyObject)
            {
                result = functions->C_DestroyObject(mySession->GetSessionHandle(), handle);
            }
            return result;
        }

        CK_RV DataObject::GetAttributeValue(AttributeList& values)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_GetAttributeValue)
            {
                // the first step just gets the sizes of all the elements
                result = functions->C_GetAttributeValue(mySession->GetSessionHandle(), handle, values.GetAttributes(), values.GetCount());
                if(result == CKR_OK)
                {
                    // resize each storage item to be able to hold the value.
                    values.ReserveStorage();
                    // recall to copy the data
                    result = functions->C_GetAttributeValue(mySession->GetSessionHandle(), handle, values.GetAttributes(), values.GetCount());
                }
            }
            return result;
        }

        CK_RV DataObject::SetAttributeValue(const AttributeList& value)
        {
            CK_RV result = CKR_FUNCTION_NOT_SUPPORTED;
            if(functions && functions->C_SetAttributeValue)
            {
                result = functions->C_SetAttributeValue(mySession->GetSessionHandle(), handle, value.GetAttributes(), value.GetCount());
            }
            return result;
        }

        void cqp::p11::AttributeList::Set(CK_ATTRIBUTE_TYPE type, const PSK& value)
        {
            __try
            {
                // prepare the storage
                Set(type);
                // allocate/find the storage slot
                auto& currentStorage = valueStorage[type];
                // copy the data into the storage
                currentStorage.value.assign(value.begin(), value.end());
                // setup the attribute fields, this will exists because of caling Set(type) above
                currentStorage.attribute->type = type;
                currentStorage.attribute->pValue = currentStorage.value.data();
                currentStorage.attribute->ulValueLen = currentStorage.value.size();
            } // try
            CATCHLOGERROR
        }

        bool cqp::p11::AttributeList::Get(CK_ATTRIBUTE_TYPE type, std::string& output)
        {
            bool result = false;
            __try
            {
                auto it = valueStorage.find(type);
                if(it != valueStorage.end() && it->second.attribute)
                {
                    output.assign(static_cast<char*>(it->second.attribute->pValue), it->second.attribute->ulValueLen);
                    result = true;
                }
                return result;
            } // try
            CATCHLOGERROR
            return result;

        }  // AttributeList::Set


    } // namespace p11

} // namespace cqp
