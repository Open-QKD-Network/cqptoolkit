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
#include "Algorithms/Datatypes/URI.h"
#include "KeyManagement/KeyStores/HSMStore.h"
#include "KeyManagement/KeyStores/YubiHSM.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "grpcpp/create_channel.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Datatypes/URI.h"

namespace cqp
{
    OpenSSLHandler* OpenSSLHandler::instance = nullptr;

    OpenSSLHandler::OpenSSLHandler()
    {
        ConsoleLogger::Enable();
        DefaultLogger().SetOutputLevel(LogLevel::Trace);
    }

    bool OpenSSLHandler::GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel, keygen::UserType& login, std::string& pin)
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

    unsigned int OpenSSLHandler::ServerCallback(SSL*, const char* identity, unsigned char* psk, unsigned int max_psk_len)
    {
        using namespace cqp;
        using namespace cqp::keygen;

        LOGTRACE("Got identity: " + std::string(identity));
        unsigned int result = 0;

        URI identityUri(identity);

        if(identityUri.GetScheme() == "pkcs")
        {
            std::map<std::string, std::string> pathElements;
            identityUri.ToDictionary(pathElements);
            const std::string destination = pathElements["object"];

            if(activeHsm)
            {
                uint64_t keyId = 0;

                if(identityUri.GetFirstParameter("id", keyId))
                {
                    LOGTRACE("Have ID=" + std::to_string(keyId) + " Destination=" + destination);
                    PSK keyValue;
                    if(activeHsm->GetKey(destination, keyId, keyValue) && keyValue.size() <= max_psk_len)
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
            else if(!keystoreAddress.empty())
            {
                KeyID keyId = 0;
                PSK keyValue;

                if(identityUri.GetFirstParameter("id", keyId) && keyId != 0)
                {
                    if(GetKeystoreKey(destination, keyId, keyValue))
                    {
                        // copy the key into the provided storage
                        std::copy(keyValue.begin(), keyValue.end(), psk);
                        result = keyValue.size();
                    }
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
            LOGERROR("Unknown identity URL: " + identity);
        }

        LOGTRACE("Leaving");
        return result;
    }

    void OpenSSLHandler::SetSearchModules(const char** modules, unsigned int numModules)
    {
        searchModules.clear();
        searchModules.reserve(numModules);
        for(unsigned int index = 0; index < numModules; index++)
        {
            searchModules.push_back(modules[index]);
        }
    }

    unsigned int OpenSSLHandler::ClientCallback(SSL*, const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len)
    {
        using namespace cqp;
        using namespace cqp::keygen;
        unsigned int result = 0;

        LOGTRACE("hint=" + hint);
        if(activeHsm)
        {
            LOGDEBUG("Using existing HSM");
            uint64_t keyId = 0;

            PSK keyValue;
            if(activeHsm->FindKey(hint, keyId, keyValue) && keyValue.size() <= max_psk_len)
            {
                std::copy(keyValue.begin(), keyValue.end(), psk);
                std::string keyIdString = "pkcs:object=" + activeHsm->GetSource() + "?id=" + std::to_string(keyId);
                keyIdString.copy(identity, max_identity_len);
                result = keyValue.size();
                LOGTRACE("Key identity=" + identity);
            } // if key found

        }
        else if(!keystoreAddress.empty())
        {
            KeyID keyId = 0;
            PSK keyValue;

            if(GetKeystoreKey(hint, keyId, keyValue))
            {
                // copy the key into the provided storage
                std::copy(keyValue.begin(), keyValue.end(), psk);

                std::string keyIdString = "pkcs:object=" + keystoreAddress + "?id=" + std::to_string(keyId);
                keyIdString.copy(identity, max_identity_len);
                result = keyValue.size();
            }
        }
        else
        {
            LOGDEBUG("Looking for a HSM");
            for(auto& token : HSMStore::FindTokens(searchModules))
            {
                LOGTRACE("Found Token");

                HSMStore store(token, pinCallback);

                uint64_t keyId = 0;
                PSK keyValue;
                if(store.FindKey(hint, keyId, keyValue) && keyValue.size() <= max_psk_len)
                {
                    std::copy(keyValue.begin(), keyValue.end(), psk);
                    std::string keyIdString = "pkcs:object=" + store.GetSource() + "?id=" + std::to_string(keyId);
                    keyIdString.copy(identity, max_identity_len);
                    result = keyValue.size();
                    LOGTRACE("Key identity=" + identity);
                    break; // for
                } // if key found

            } // for tokens
        }
        LOGTRACE("Leaving");
        return result;
    }

    void OpenSSLHandler::SetPinCallback(OpenSSLHandler_PinCallback cb, void* userData)
    {
        // we're passing through to a C function so use the dummy callback handler
        pinCallback = this;
        pinCallbackFunc = cb;
        callbackUserData = userData;
    }

    void OpenSSLHandler::SetPinCallback(keygen::IPinCallback* cb)
    {
        pinCallback = cb;
        // clear any C callbacks
        pinCallbackFunc = nullptr;
        callbackUserData = nullptr;
    }

    unsigned OpenSSLHandler::SetHSM(const char* url)
    {
        using namespace cqp;
        LOGTRACE("");
        bool result = false;

        delete activeHsm;
        activeHsm = nullptr;

        URI hsmUri(url);
        if(hsmUri.GetScheme() == "pkcs")
        {
            if(std::string(url).find("yubihsm") != std::string::npos)
            {
                activeHsm = new cqp::keygen::YubiHSM(url, pinCallback);
            }
            else
            {
                activeHsm = new cqp::keygen::HSMStore(url, pinCallback);
            }
            result = activeHsm->InitSession();
        }
        else
        {
            keystoreAddress = url;
            result = true;
        }

        return result;
    }

    bool OpenSSLHandler::GetKeystoreKey(const std::string& destination, KeyID& keyId, PSK& psk)
    {
        bool result = false;
        auto channel = grpc::CreateChannel(keystoreAddress, grpc::InsecureChannelCredentials());
        auto stub = remote::IKey::NewStub(channel);
        grpc::ClientContext ctx;
        remote::KeyRequest request;
        remote::SharedKey key;
        request.set_siteto(destination);
        request.set_keyid(keyId);
        if(LogStatus(stub->GetSharedKey(&ctx, request, &key)).ok())
        {
            psk.clear();
            psk.assign(key.keyvalue().cbegin(), key.keyvalue().end());
            keyId = key.keyid();

            result = true;
        }
        return result;
    }

}

using namespace cqp;

unsigned int OpenSSLHandler_ServerCallback(SSL* ssl, const char* identity, unsigned char* psk, unsigned int max_psk_len)
{
    return OpenSSLHandler::Instance()->ServerCallback(ssl, identity, psk, max_psk_len);
} // OpenSSLHandler_ServerCallback

#if (OPENSSL_VERSION_NUMBER >= 0x010101000L)
int OpenSSLHandler_SessionCallback(SSL*, const EVP_MD* md, const unsigned char** id, size_t* idlen, SSL_SESSION** sess)
{
    // md will be NULL on first invocation for a connection
    // if called again, it will contain the digest for the chosen cipher suite

    // TODO
    return 0;
}
#endif

void OpenSSLHandler_SetSearchModules(const char** modules, unsigned int numModules)
{
    OpenSSLHandler::Instance()->SetSearchModules(modules, numModules);
}

unsigned int OpenSSLHandler_ClientCallback(SSL* ssl, const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len)
{
    return OpenSSLHandler::Instance()->ClientCallback(ssl, hint, identity, max_identity_len, psk, max_psk_len);
}

void OpenSSLHandler_SetPinCallback(OpenSSLHandler_PinCallback cb, void* userData)
{
    OpenSSLHandler::Instance()->SetPinCallback(cb, userData);
}

unsigned OpenSSLHandler_SetHSM(const char* url)
{
    return OpenSSLHandler::Instance()->SetHSM(url);
}
