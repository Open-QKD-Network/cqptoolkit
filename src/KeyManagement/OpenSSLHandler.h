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
#pragma once
#ifdef __cplusplus
#include "KeyManagement/KeyStores/HSMStore.h"
extern "C" {
#endif

#include "KeyManagement/keymanagement_export.h"
#include "openssl/ssl.h"

/**
 * @brief OpenSSLHandler_ServerCallback
 * Supplies OpenSSL with the correct PSK on request. For TLS <= 1.2. Attach with
 * ``SSL_CTX_set_psk_server_callback`` or ``SSL_set_psk_server_callback``
 * @see https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_set_psk_server_callback.html
 * @param ssl
 * @param identity
 * @param psk
 * @param max_psk_len
 * @return 0 on failure, otherwise the length of the PSK
 */
KEYMANAGEMENT_EXPORT unsigned int OpenSSLHandler_ServerCallback(SSL *ssl, const char* identity,
        unsigned char *psk, unsigned int max_psk_len);

/**
 * @brief OpenSSLHandler_ClientCallback
 * Supplies OpenSSL with the correct PSK on request. For TLS <= 1.2. Attach with
 * ``SSL_CTX_set_psk_client_callback`` or ``SSL_set_psk_client_callback``
 * @see https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_set_psk_client_callback.html
 * @param ssl
 * @param hint
 * @param identity
 * @param max_identity_len
 * @param psk
 * @param max_psk_len
 * @return 0 on failure, otherwise the length of the PSK
 */
KEYMANAGEMENT_EXPORT unsigned int OpenSSLHandler_ClientCallback(SSL *ssl, const char *hint,
        char* identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len);

#if (OPENSSL_VERSION_NUMBER >= 0x010101000L)
/**
 * @brief OpenSSLHandler_SessionCallback
 * Supplies OpenSSL with the correct PSK on request. For TLS >= 1.3. Attach with
 * ``SSL_CTX_set_psk_use_session_callback`` or ``SSL_set_psk_use_session_callback``
 * @param ssl_conn
 * @param md
 * @param id
 * @param idlen
 * @param sess
 * @return 1 on success, 0 on failure
 */
KEYMANAGEMENT_EXPORT int OpenSSLHandler_SessionCallback(SSL *ssl, const EVP_MD *md,
        const unsigned char **id,
        size_t *idlen,
        SSL_SESSION **sess);
#endif

/**
 * @brief OpenSSLHandler_SetSearchModules
 * Specifies which libraries to use when looking for usable tokens
 * @param modules Array of strings containing module names/paths
 * @param numModules The number of elements in modules
 */
KEYMANAGEMENT_EXPORT void OpenSSLHandler_SetSearchModules(const char** modules, unsigned int numModules);

/**
 * Callback to supply the pin for a token when required
 * @param[in] userData The value provided when SetPinCallback was called
 * @param[in] tokenSerial The serial number of the token being accessed
 * @param[in] tokenName The name of the token being accessed
 * @param[out] userTypeOut Destination for the kind of login that should be used - defaults to User
 * @param[out] pinOut Destination for the pin to use to log in. This has been allocated before the call.
 * @param[in] pinOutMax The maximum number of bytes that pinOut can hold.
 * @return The length of pinOut or 0 on failure
 */
typedef size_t (*OpenSSLHandler_PinCallback)(void* userData, const char* tokenSerial, const char* tokenName, unsigned long* userTypeOut, char* pinOut, size_t pintOutMax);

/**
 * @brief SetPin
 * Specify a callback to provide password/pin for a token when needed. The callback can then lookup/request the pin from the user.
 * @param cb The callback to call
 * @param userData This will be passed to the callback
 */
KEYMANAGEMENT_EXPORT void OpenSSLHandler_SetPinCallback(OpenSSLHandler_PinCallback cb, void* userData);

/**
 * @brief OpenSSLHandler_SetHSM
 * sets the HSM to use for future use of OpenSSLHandler_ClientCallback and OpenSSLHandler_ServerCallback
 * @details
 * If the url does not start with "pkcs" it is assumed to be an IKey interface hostname
 * @param url pkcs url for module or IKey interface
 * @return non-zero on success, 0 on failure
 */
KEYMANAGEMENT_EXPORT unsigned OpenSSLHandler_SetHSM(const char* url);

/**
 * @brief OpenSSLHandler_SetKeystore
 * sets the key store to use for future use of OpenSSLHandler_ClientCallback and OpenSSLHandler_ServerCallback
 * This overrides any call to OpenSSLHandler_SetHSM
 * @param address The address of the IKey interface to connect to.
 * @return non-zero on success, 0 on failure
 */
KEYMANAGEMENT_EXPORT unsigned OpenSSLHandler_SetKeystore(const char* address);

#ifdef __cplusplus
} // extern "C"

namespace cqp
{

    class KEYMANAGEMENT_EXPORT OpenSSLHandler final : public virtual keygen::IPinCallback
    {
    public:
        inline static OpenSSLHandler* Instance()
        {
            if(!instance)
            {
                instance = new OpenSSLHandler();
            }
            return instance;
        }

        OpenSSLHandler();

        inline bool GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel,
                              keygen::UserType& login, std::string& pin) override;

        void SetSearchModules(const char** modules, unsigned int numModules);

        void SetPinCallback(OpenSSLHandler_PinCallback cb, void* userData);

        void SetPinCallback(cqp::keygen::IPinCallback* cb);

        unsigned SetHSM(const char* url);

        unsigned int ServerCallback(SSL*, const char* identity, unsigned char* psk, unsigned int max_psk_len); // ServerCallback

        unsigned int ClientCallback(SSL*, const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len); // ClientCallback

        bool GetKeystoreKey(const std::string& destination, KeyID& keyId, PSK& psk);
    protected:
        std::vector<std::string> searchModules = { "libsofthsm2.so" };
        OpenSSLHandler_PinCallback pinCallbackFunc = nullptr;
        void* callbackUserData = nullptr;
        size_t pinLengthLimit = 0;
        cqp::keygen::HSMStore* activeHsm = nullptr;
        std::string keystoreAddress;
        cqp::keygen::IPinCallback* pinCallback = nullptr;

    private:
        static OpenSSLHandler* instance;
    };

}

#endif
