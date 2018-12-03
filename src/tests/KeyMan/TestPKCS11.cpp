/*!
* @file
* @brief TestKeyMan
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 9/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestPKCS11.h"
#include "KeyManagement/KeyStores/PKCS11Wrapper.h"
#include "KeyManagement/KeyStores/HSMStore.h"

#define YH_ALGO_AES256_CCM_WRAP 42

/* This is an offset for the vendor definitions to avoid clashes */
#define YUBICO_BASE_VENDOR 0x59554200

#define CKK_YUBICO_AES128_CCM_WRAP                                             \
    (CKK_VENDOR_DEFINED | YUBICO_BASE_VENDOR | YH_ALGO_AES128_CCM_WRAP)
#define CKK_YUBICO_AES192_CCM_WRAP                                             \
    (CKK_VENDOR_DEFINED | YUBICO_BASE_VENDOR | YH_ALGO_AES192_CCM_WRAP)
#define CKK_YUBICO_AES256_CCM_WRAP                                             \
    (CKK_VENDOR_DEFINED | YUBICO_BASE_VENDOR | YH_ALGO_AES256_CCM_WRAP)

#define CKM_YUBICO_AES_CCM_WRAP                                                \
    (CKM_VENDOR_DEFINED | YUBICO_BASE_VENDOR | YH_WRAPKEY)

namespace cqp
{
    namespace tests
    {
        TEST(KeyMan, PKCS)
        {
            using namespace p11;
            using std::shared_ptr;

            //auto module = Module::Create("libsofthsm2.so");
            auto module = Module::Create("libsofthsm2.so");

            ASSERT_NE(module, nullptr) << "Module init failed";
            SlotList slots;
            ASSERT_EQ(CheckP11(module->GetSlotList(true, slots)), CKR_OK) << "Failed to get slots";
            for(auto slotId : slots)
            {
                // HACK: softhsm2 reports a token with id 1, but it's not valid.
                if(slotId != 1)
                {
                    shared_ptr<Slot> slot(new Slot(module, slotId));
                    auto session = Session::Create(slot);
                    CK_TOKEN_INFO tokenInfo {};
                    ASSERT_EQ(slot->GetTokenInfo(tokenInfo), CKR_OK) << "Fail to get info";
                    ASSERT_NE(tokenInfo.flags & CKF_TOKEN_INITIALIZED, 0) << "Token not initialised";
                    ASSERT_EQ(tokenInfo.flags & CKF_WRITE_PROTECTED, 0) << "Token write protected";
                    ASSERT_EQ(CheckP11(session->Login(CKU_USER, "1234")), CKR_OK) << "Failed to login";

                    p11::DataObject obj(session);
                    AttributeList attribList;
                    attribList.Set(CKA_CLASS, CKO_SECRET_KEY);
                    attribList.Set(CKA_KEY_TYPE, CKK_GENERIC_SECRET);
                    attribList.Set(CKA_TOKEN, true);
                    attribList.Set(CKA_ID, 3);
                    attribList.Set(CKA_LABEL, "Test");
                    attribList.Set(CKA_VALUE, "12031029312031029312031029345");
                    //attribList.Set(CKA_EXTRACTABLE, true);

                    ASSERT_EQ(CheckP11(obj.CreateObject(attribList)), CKR_OK) << "Failed to create key";

                    AttributeList searchParams;
                    searchParams.Set(CKA_LABEL, "Test");
                    ObjectList results;
                    ASSERT_EQ(session->FindObjects(searchParams, 1, results), CKR_OK) << "Search failed";
                    ASSERT_EQ(results.size(), 1) << "Wrong number of results";
                    results.clear();

                    ASSERT_EQ(session->FindObjects(searchParams, 100, results), CKR_OK) << "Search failed";
                    ASSERT_GE(results.size(), 1) << "Wrong number of results";


                    ASSERT_EQ(CheckP11(obj.DestroyObject()), CKR_OK) << "Failed to destroy key";
                }
            }
        }

        TEST(KeyMan, HSM)
        {
            const std::string dest = "siteB:654";
            keygen::HSMStore store("pkcs:module-name=libsofthsm2.so;token=SoftHSM2Token?pin-value=1234");
            keygen::IBackingStore::Keys keys;
            keys.push_back({1003, {185, 182, 156, 211,  87, 183,  52, 248,  47, 214, 120, 101,  47,  71, 154, 186,
                                   103,  36, 132, 218, 119, 190,  28, 185,  89, 168,  29, 124,  29, 211, 132, 210
                                  }});
            ASSERT_TRUE(store.StoreKeys(dest, keys)) << "Key storage failed";
            PSK foundKey;
            ASSERT_TRUE(store.FindKey(dest, keys[0].first, foundKey)) << "Failed to find key";
            KeyID nextId = 0;
            ASSERT_TRUE(store.ReserveKey(dest, nextId)) << "Failed to reserve key";
            PSK retrievedKey;
            ASSERT_TRUE(store.RemoveKey(dest, nextId, retrievedKey)) << "RemoveKey Failed";
        }

        TEST(KeyMan, HSMUrl)
        {
            const std::string dest = "siteB:654";
            keygen::HSMStore store("pkcs:module-name=libsofthsm2.so;token=SoftHSM2Token?pin-value=1234");
            keygen::IBackingStore::Keys keys;
            keys.push_back({1003, {185, 182, 156, 211,  87, 183,  52, 248,  47, 214, 120, 101,  47,  71, 154, 186,
                                   103,  36, 132, 218, 119, 190,  28, 185,  89, 168,  29, 124,  29, 211, 132, 210
                                  }});
            ASSERT_TRUE(store.StoreKeys(dest, keys)) << "Key storage failed";
            KeyID nextId = 0;
            ASSERT_TRUE(store.ReserveKey(dest, nextId)) << "Failed to reserve key";
            PSK retrievedKey;
            ASSERT_TRUE(store.RemoveKey(dest, nextId, retrievedKey)) << "RemoveKey Failed";
        }

    }
}
