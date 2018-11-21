#include "BackingStoreFactory.h"
#include "CQPToolkit/KeyGen/YubiHSM.h"
#include "CQPToolkit/KeyGen/HSMStore.h"
#include "CQPToolkit/KeyGen/FileStore.h"
#include "CQPToolkit/Util/Util.h"

namespace cqp {
    namespace keygen {

        std::shared_ptr<IBackingStore> BackingStoreFactory::CreateBackingStore(const std::string& url)
        {
            URI bsUrl(url);


            std::shared_ptr<IBackingStore> result;

            const auto backingStoreType = ToLower(bsUrl.GetScheme());
#if defined(SQLITE3_FOUND)
            if(backingStoreType == "file")
            {
                std::string filename = bsUrl.GetPath();
                if(filename.empty())
                {
                    filename = "keys.db";
                }
                result.reset(new keygen::FileStore(filename));
            } else
#endif
            if(backingStoreType == "pkcs11")
            {
                result.reset(new keygen::HSMStore(url));
            }
            else if(backingStoreType == "yubihsm2")
            {
                result.reset(new keygen::YubiHSM(bsUrl));
            }
            else if(backingStoreType.empty())
            {
                // leave as null
            }
            else {
                LOGERROR("Unsupported backingstore: " + backingStoreType);
            }
            return result;
        }

    } // namespace keygen
} // namespace cqp
