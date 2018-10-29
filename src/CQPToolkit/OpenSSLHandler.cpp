/*!
* @file
* @brief OpenSSLHandler
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 17/8/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "OpenSSLHandler.h"
#include "CQPToolkit/Util/URI.h"
#include "CQPToolkit/KeyGen/HSMStore.h"
#include "CQPToolkit/Util/ConsoleLogger.h"
#include "CQPToolkit/Util/Util.h"

static std::vector<std::string> searchModules = { "libsofthsm2.so" };
static OpenSSLHandler_PinCallback pinCallbackFunc = nullptr;
static void* callbackUserData = nullptr;
static size_t pinLengthLimit = 0;
static cqp::keygen::HSMStore* activeHsm;

namespace cqp
{
    class DummyCallback : public virtual keygen::IPinCallback
    {
        inline bool GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel,
                              keygen::UserType& login, std::string& pin) override
        {
            bool result = false;

            if(pinCallbackFunc)
            {
                std::vector<char> tempPin(pinLengthLimit+1, 0);
                ulong loginInt = 0;
                size_t pinUsed = pinCallbackFunc(callbackUserData, tokenSerial.c_str(), tokenLabel.c_str(),
                                             &loginInt, tempPin.data(), pinLengthLimit);
                switch(loginInt)
                {
                case 0:
                    login = keygen::UserType::SecurityOfficer;
                    break;
                case 1:
                    login = keygen::UserType::User;
                    break;
                case 2:
                    login = keygen::UserType::ContextSpecific;
                    break;
                }
                result = pinUsed > 0 && pinUsed <= pinLengthLimit;
                if(result)
                {
                    pin.clear();
                    pin.assign(tempPin.begin(), tempPin.begin() + pinUsed);
                }
            }
            return result;
        }
    };
}

static cqp::DummyCallback callbackHelper;
static cqp::keygen::IPinCallback* pinCallback = nullptr;

unsigned int OpenSSLHandler_ServerCallback(SSL*, const char* identity, unsigned char* psk, unsigned int max_psk_len)
{
    using namespace cqp;
    using namespace cqp::keygen;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Trace);

    LOGTRACE("Got identity: " + std::string(identity));
    unsigned int result = 0;

    URI identityUri(identity);

    if(identityUri.GetScheme() == "pkcs")
    {
        if(activeHsm)
        {
            std::map<std::string, std::string> pathElements;
            identityUri.ToDictionary(pathElements);
            uint64_t keyId = 0;

            if(identityUri.GetFirstParameter("id", keyId))
            {
                LOGTRACE("Have ID");
                PSK keyValue;
                if(activeHsm->GetKey(pathElements["object"], keyId, keyValue) && keyValue.size() <= max_psk_len)
                {
                    std::copy(keyValue.begin(), keyValue.end(), psk);
                    result = keyValue.size();
                } // if GetKey
            } // if have id
            else
            {
                LOGERROR("No ID specified");
            }
        }
        else
        {
            LOGERROR("No active HSM");
        }

    }
    else
    {
        LOGERROR("Unknown indenty URL: " + identity);
    }

    LOGTRACE("Leaving");
    return result;
} // OpenSSLHandler_ServerCallback

#if (OPENSSL_VERSION_NUMBER >= 0x010101000L)
int OpenSSLHandler_SessionCallback(SSL*, const EVP_MD* md, const unsigned char** id, size_t* idlen, SSL_SESSION** sess)
{
    // md will be NULL on first invocation for a connection
    // if called again, it will contain the digest for the chosen ciphersuite

    // TODO
    return 0;
}
#endif

void OpenSSLHandler_SetSearchModules(const char** modules, unsigned int numModules)
{
    searchModules.clear();
    searchModules.reserve(numModules);
    for(unsigned int index = 0; index < numModules; index++)
    {
        searchModules.push_back(modules[index]);
    }
}

unsigned int OpenSSLHandler_ClientCallback(SSL*, const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len)
{
    using namespace cqp;
    using namespace cqp::keygen;
    unsigned int result = 0;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Trace);

    LOGTRACE("");
    if(activeHsm)
    {
        uint64_t keyId = 0;
        std::string destination;

        PSK keyValue;
        if(activeHsm->FindKey(hint, keyId, keyValue) && keyValue.size() <= max_psk_len)
        {
            std::copy(keyValue.begin(), keyValue.end(), psk);
            std::string keyIdString = ToHexString(keyId);
            strncpy(identity, keyIdString.c_str(), max_identity_len);
            result = keyValue.size();
        } // if key found

    }
    else
    {
        for(auto& token : HSMStore::FindTokens(searchModules))
        {
            LOGTRACE("Found Token");

            HSMStore store(token, pinCallback);

            uint64_t keyId = 0;
            std::string destination;

            PSK keyValue;
            if(store.FindKey(hint, keyId, keyValue) && keyValue.size() <= max_psk_len)
            {
                std::copy(keyValue.begin(), keyValue.end(), psk);
                std::string keyIdString = ToHexString(keyId);
                strncpy(identity, keyIdString.c_str(), max_identity_len);
                result = keyValue.size();
                break; // for
            } // if key found

        } // for tokens
    }
    LOGTRACE("Leaving");
    return result;
}

void OpenSSLHandler_SetPinCallback(OpenSSLHandler_PinCallback cb, void* userData)
{
    // we're passing through to a C function so use the dummy callback handler
    pinCallback = &callbackHelper;
    pinCallbackFunc = cb;
    callbackUserData = userData;
}

unsigned OpenSSLHandler_SetHSM(const char* url)
{
    using namespace cqp;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Trace);
    LOGTRACE("");
    if(activeHsm)
    {
        delete activeHsm;
        activeHsm = nullptr;
    }

    activeHsm = new cqp::keygen::HSMStore(url, pinCallback);
    bool result = activeHsm->InitSession();
    if(result)
    {
        return 1;
    }
    else
    {
        LOGERROR("Failed to start HSM");
        return 0;
    }
    LOGTRACE("");
}

void OpenSSLHandler_SetPinCallback(cqp::keygen::IPinCallback* cb)
{
    pinCallback = cb;
    // clear any C callbacks
    pinCallbackFunc = nullptr;
    callbackUserData = nullptr;
}
