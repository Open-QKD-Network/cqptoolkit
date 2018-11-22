#pragma once
#include <string>
#include <memory>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp {
    namespace keygen {

        class IBackingStore;

        /**
         * @brief The BackingStoreFactory class
         * Creates backing stores based on the URL
         */
        class CQPTOOLKIT_EXPORT BackingStoreFactory
        {
        public:
            /**
             * @brief CreateBackingStore
             * Create a backing store
             * @details
             * Possible valid url schemas are:
             *  - `file://<filename>` An SQLite database stored in a file
             *  - `pkcs11:<pkcs11 string>` A PKCS#11 compatible HSM
             *  - `yubihsm2:<pkcs11 string>` A YubiHSM2 device with the PKCS#11 bridge
             * @param url The address for the backing store
             * @return A backing store object or null if the URL is invalid
             */
            static std::shared_ptr<IBackingStore> CreateBackingStore(const std::string& url);
        };

    } // namespace keygen
} // namespace cqp


