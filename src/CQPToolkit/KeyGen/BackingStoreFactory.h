#pragma once
#include <string>
#include <memory>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp {
    namespace keygen {

        class IBackingStore;

        class CQPTOOLKIT_EXPORT BackingStoreFactory
        {
        public:
            BackingStoreFactory() = default;

            static std::shared_ptr<IBackingStore> CreateBackingStore(const std::string& url);
        };

    } // namespace keygen
} // namespace cqp


