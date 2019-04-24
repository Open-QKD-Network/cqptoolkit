#include "BackingStoreFactory.h"
#include "KeyManagement/KeyStores/YubiHSM.h"
#include "KeyManagement/KeyStores/HSMStore.h"
#include "KeyManagement/KeyStores/FileStore.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Logging/Logger.h"

namespace cqp
{
    namespace keygen
    {

        std::shared_ptr<IBackingStore> BackingStoreFactory::CreateBackingStore(const std::string& url)
        {
            URI bsUrl(url);
            LOGDEBUG("Creating a backing store for " + url);

            std::shared_ptr<IBackingStore> result;

            const auto backingStoreType = ToLower(bsUrl.GetScheme());
#if defined(SQLITE3_FOUND)
            if(backingStoreType == "file")
            {
                // a file name looks like a host to the parser so prefix this to the full path
                std::string filename = bsUrl.GetHost() + bsUrl.GetPath();
                if(filename.empty())
                {
                    LOGDEBUG("Using default filename: keys.db");
                    filename = "keys.db";
                }
                result.reset(new keygen::FileStore(filename));
            }
            else
#endif
                if(backingStoreType == "pkcs11")
                {
                    result.reset(new keygen::HSMStore(url));
                }
                else if(backingStoreType == "yubihsm2")
                {
                    result.reset(new keygen::YubiHSM(bsUrl));
                }
                else if(url.empty())
                {
                    // leave as null
                }
                else
                {
                    LOGERROR("Unsupported backingstore: " + backingStoreType);
                }
            return result;
        }

    } // namespace keygen
} // namespace cqp
