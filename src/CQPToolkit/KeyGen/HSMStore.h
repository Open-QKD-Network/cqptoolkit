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
#pragma once
#include <memory>                      // for shared_ptr
#include <bits/stdint-uintn.h>                    // for uint64_t
#include <stddef.h>                               // for size_t
#include <limits>                                 // for numeric_limits
#include <string>                                 // for string
#include <vector>                                 // for vector
#include "CQPToolkit/Interfaces/IBackingStore.h"  // for IBackingStore::Keys
#include "CQPToolkit/Util/URI.h"                  // for URI
#include "CQPToolkit/Datatypes/Keys.h"                       // for KeyID, PSK (ptr only)
#include <chrono>

namespace cqp
{
    namespace p11
    {
        class Module;
        class Slot;
        class Session;
        class AttributeList;
    }

    namespace keygen
    {
        enum class UserType : unsigned long { SecurityOfficer = 0, User = 1, ContextSpecific = 3 };

        /**
         * @brief The IPinCallback class
         * Callback for providing a pin to access a HSM
         */
        class CQPTOOLKIT_EXPORT IPinCallback
        {
        public:
            /**
             * @brief GetHSMPin
             * @param tokenSerial The serial number of the token being accessed
             * @param tokenLabel The name of the token being accessed
             * @param[out] login Destination for the kind of login that should be used - defaults to User
             * @param[out] pin Destination for the pin to use to log in.
             * @return true on successfully oibtaining a pin
             */
            virtual bool GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel,
                                   UserType& login, std::string& pin) = 0;

            /// distructor
            virtual ~IPinCallback() = default;
        };

        /**
         * @brief The HSMStore class
         * Provide an IBackingStore interface to standard PPKCS#11 hardware security modules
         */
        class CQPTOOLKIT_EXPORT HSMStore : public virtual IBackingStore
        {
        public:
            /**
             * @brief HSMStore
             * Contruct a store from the PKCS url. Path and property values are percent encoded.
             * Paths are seperated by ``;``
             * See [RFC7512](https://tools.ietf.org/html/rfc7512) for details.
             *
             * The properties of the  url __must__ contain:
             * __one of__ these path elements to defined the library to use:
             * - ``module-name`` - library name
             * - ``module-path`` - full path to library
             *
             * __One of__ these path elements to define which token to use:
             * - ``serial`` - Serialnumber for a token
             * - ``token`` - Token Label defined during initialisation
             * - ``slot-id`` - Slot index number for the token
             *
             * __One of__ these query values to define the pin code:
             * - ``pin-value`` - The percent encoded pin value
             * - ``pin-source`` - The filename to read the pin from.
             *
             * Optionally these query values
             * - ``login`` - force the login type to Security officer, valid values are ``user``, ``so``, ``cs``
             * - ``source`` - The source location for this HSM to define where keys have come from (not PKCS#11 standard)
             *
             * **Examples**
             *
             * __SoftHSM2 with a token named "my token" with a pin "1234"__
             *
             * ``pkcs11:module-name=libsofthsm2.so;token=my%20token?pin-value=1234``
             *
             * __AcmeCorp with token serial number "828882727" using a pin from file ``/etc/secret.pin``__
             *
             * ``pkcs11:module-path=/opt/AcmeCorp/pkcs11.so;serial=828882727?pin-source=/etc/secret.pin``
             * @param pkcsUrl a PKCS#11 url
             * @param callback Callback for providing the pin to login when needed
             */
            HSMStore(const URI& pkcsUrl, IPinCallback* callback = nullptr, const void* moduleLoadOptions = nullptr);

            /**
             * @brief SetPinCallback
             * Sets the object to called when a pin is required to log in
             * @param cb The call back to call to get the pin
             */
            inline void SetPinCallback(IPinCallback* cb)
            {
                pinCallback = cb;
            }

            /// A list of urls for found tokens
            using FoundTokenList = std::vector<URI>;

            /**
             * @brief FindTokens
             * Try to find usable tokens with the supplied modules
             * @param modules Modules to try
             * @return Any found tokens as a list of pkcs URIs
             */
            static FoundTokenList FindTokens(const std::vector<std::string>& modules);

            /// Distructor
            virtual ~HSMStore() override;

            /**
             * @brief GetKey
             * Get the key data for a specific key
             * @param destination The paired site
             * @param keyId which key to get
             * @param[out] output the output for the key material
             * @return true if the key was found
             */
            bool GetKey(const std::string& destination, KeyID keyId, PSK& output);

            /**
             * @brief FindKey
             * Find a key for a destination
             * @param destination The paired site
             * @param[in,out] keyId The ID to find or, if this is 0, on success it will contain the chosen key's id
             * @param[out] outputoutput the output for the key material
             * @return true if a key was found
             */
            bool FindKey(const std::string& destination, KeyID& keyId, PSK& output);

            /**
             * @brief KeyExists
             * Try to find the key with the given id
             * @param destination The paired site
             * @param keyId The ID to find
             * @return true on success
             */
            bool KeyExists(const std::string& destination, KeyID keyId);

            /**
             * @brief GetPinLengthLimit
             * @return the maximum length of the pin for this device.
             */
            inline size_t GetPinLengthLimit() const
            {
                return pinLengthLimit;
            }

            /**
             * @brief InitSession
             * prepare the session for use
             * @return true on success
             */
            bool InitSession();

            /**
             * @brief DeleteAllKeys
             * Delete all objects which match the type stored by this class
             * @return true on success
             */
            unsigned int DeleteAllKeys();

            /**
             * @brief GetSource
             * @return The source identigier for all keys
             */
            std::string GetSource() const {
                return source;
            }

            /// @copydoc IBackingStore::RemoveKey
            bool RemoveKey(const std::string& destination, KeyID keyId);

            /// @{
            /// @name IBackingStore interface
        public:
            /// @copydoc IBackingStore::StoreKeys
            bool StoreKeys(const std::string& destination, Keys& keys) override;

            /// @copydoc IBackingStore::RemoveKey
            bool RemoveKey(const std::string& destination, KeyID keyId, PSK& output) override;

            /// @copydoc IBackingStore::RemoveKeys
            bool RemoveKeys(const std::string& destination, Keys& keys) override;

            /// @copydoc IBackingStore::ReserveKey
            bool ReserveKey(const std::string& destination, KeyID& keyId) override;

            /// @copydoc IBackingStore::GetCounts
            void GetCounts(const std::string& destination, uint64_t& availableKeys, uint64_t& remainingCapacity) override;

            /// @copydoc IBackingStore::GetNextKeyId
            uint64_t GetNextKeyId(const std::string& destination) override;

            ///@}
            ///
        protected: // members
            /// The library used to access the HSM
            std::shared_ptr<p11::Module> module;
            /// The interface to the token
            std::shared_ptr<p11::Slot> slot;
            /// current session
            std::shared_ptr<p11::Session> session;

            /// The slot number
            ulong slotId = 0;
            /// readable name of the token
            std::string tokenLabel;
            /// serial number of the token
            std::string serial;
            /// filename of pin
            std::string pinSource;
            /// pin to log in with
            std::string pinValue;
            /// tokens limit for pin
            size_t pinLengthLimit = std::numeric_limits<size_t>::max();
            /// who to log in
            UserType login = UserType::User;
            /// where do our keys come from
            std::string source;
            /// if slot id has been set
            bool slotIdValid = false;
            /// source of pin
            IPinCallback* pinCallback = nullptr;
            /// defaults for new objects
            std::unique_ptr<p11::AttributeList> newObjDefaults;
            /// defaults for searching for objects
            std::unique_ptr<p11::AttributeList> findObjDefaults;

            const std::chrono::system_clock::time_point zeroStartDate = std::chrono::system_clock::from_time_t(0);
            /// The size of the keyID storage (device dependent)
            size_t bytesPerKeyID = sizeof(uint64_t);
            using SessionHandle = ulong;
            using Notification = ulong;

        protected: // methods
            /**
             * prepare the token for use
             * @return true on success
             */
            bool InitSlot();

            /**
             * @brief SessionEventCallback
             * Recieves callbacks from the pkcs11 layer
             * @param hSession
             * @param event
             * @param pApplication
             * @return result code
             */
            static ulong SessionEventCallback(
                SessionHandle hSession,     /* the session's handle */
                Notification   event,
                void*       pApplication  /* passed to C_OpenSession */);

            /**
             * @brief SetID
             * @param attrList
             * @param keyId
             */
            void SetID(p11::AttributeList& attrList, KeyID keyId);

            /**
             * @brief FixKeyID
             * @param keyId
             */
            void FixKeyID(KeyID& keyId);
        }; //

    } // namespace keygen
} // namespace cqp


