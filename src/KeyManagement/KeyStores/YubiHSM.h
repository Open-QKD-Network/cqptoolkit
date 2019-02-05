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
#pragma once
#include <string>
#include <KeyManagement/KeyStores/HSMStore.h>
#include <memory>
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/Keys.h"

namespace cqp
{
    namespace keygen
    {
        /**
         * @brief The YubiHSM class
         * This provides easy access to the YubiHSM2 device
         * This device is not a capable or true HSM so some features are handled in class.
         * The keys can only be stored with a 2 byte ID and a 40 byte label with no other meta data.
         *
         * **Example pkcs11 urls**
         *
         * ``pkcs11:module-name=/usr/lib/x86_64-linux-gnu/pkcs11/yubihsm_pkcs11.so?pin-value=0001password``
         */
        class KEYMANAGEMENT_EXPORT YubiHSM : public HSMStore
        {
        public:
            /// Load options to pass to the driver
            static CONSTSTRING DefaultLoadOptions = "connect=http://localhost:12345\ndebug\nlibdebug\ndinout";

            /**
             * @brief YubiHSM constructor
             * @param pkcsUrl URL of the device
             * @param callback Where to get a pin from if required
             * @param loadOptions Options to pass to the driver
             */
            YubiHSM(const std::string& pkcsUrl, IPinCallback* callback = nullptr,
                    const std::string& loadOptions = DefaultLoadOptions);

            /// Distructor
            virtual ~YubiHSM() override = default;

            ///@{
            /// @name HSMStore overrides

            /// @copydoc HSMStore::ReserveKey
            /// @details YubiHSM2 does not permit storing any metadata, so they reservation is only stored internally
            bool ReserveKey(const std::string& destination, KeyID& keyId) override;

            /// @copydoc HSMStore::RemoveKey
            bool RemoveKey(const std::string& destination, KeyID keyId, PSK& output) override;

            /// @}
        protected: // members
            /// Keys currently held for other requests
            std::map<std::string, std::vector<KeyID>> reservedKeys;
        }; //class YubiHSM

    } // namespace keygen
} // namespace cqp


