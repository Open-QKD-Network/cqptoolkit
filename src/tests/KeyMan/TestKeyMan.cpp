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
#include "TestKeyMan.h"
#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include <grpcpp/server_builder.h>
#include <grpcpp/server.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "KeyManagement/KeyStores/FileStore.h"
#include "Algorithms/Util/FileIO.h"

namespace cqp
{
    namespace tests
    {
        TEST(KeyMan, FileStore)
        {
            const uint numberOfKeys = 10000;
            const std::string dest = "SiteB";
            keygen::IBackingStore::Keys keyData;
            PSK aKey;
            for(uint i = 0; i < numberOfKeys; i++)
            {
                for(uint8_t i = 0; i < 32; i++)
                {
                    aKey.push_back(rand());
                }
                keyData.push_back({i, aKey});
                aKey.clear();
            }

            fs::Delete("FileStoreTest.db");
            keygen::FileStore fileStore("FileStoreTest.db");

            auto start = std::chrono::high_resolution_clock::now();

            auto keyDataCopy = keyData;
            fileStore.StoreKeys(dest, keyDataCopy);

            auto timeTaken = std::chrono::high_resolution_clock::now() - start;
            uint timeTakenms = std::chrono::duration_cast<std::chrono::milliseconds>(timeTaken).count();
            const uint keysPerSecond = numberOfKeys * (1.0 / (timeTakenms / 1000.0));
            LOGINFO(std::to_string(numberOfKeys) + " Key Storage took:" + std::to_string(timeTakenms) + "ms, " + std::to_string(keysPerSecond) + " keys per second.");


            for(uint i = 0; i < numberOfKeys; i++)
            {
                PSK key;
                ASSERT_TRUE(fileStore.RemoveKey(dest, i, key));
                ASSERT_EQ(key, keyData[i].second);
                key.clear();
            }
        }

        TEST(KeyMan, Factory)
        {
            //create factory
            keygen::KeyStoreFactory factory1(grpc::InsecureChannelCredentials());
            keygen::KeyStoreFactory factory2(grpc::InsecureChannelCredentials());

            // create the server for second factory
            grpc::ServerBuilder builder;
            int server2ListenPort = 0;
            builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &server2ListenPort);
            builder.RegisterService(static_cast<remote::IKeyFactory::Service*>(&factory2));
            auto server2 = builder.BuildAndStart();

            // set address/identity
            factory1.SetSiteAddress("localhost:0");
            factory2.SetSiteAddress("localhost:" + std::to_string(server2ListenPort));

            ASSERT_NE(server2, nullptr);

            // create keystores
            auto keyStore1 = factory1.GetKeyStore("localhost:" + std::to_string(server2ListenPort));
            auto keyStore2 = factory2.GetKeyStore("localhost:0");

            // load a key to retrieve
            KeyList dummyKey;
            dummyKey.push_back({42, 3, 2, 1});

            keyStore1->OnKeyGeneration(std::unique_ptr<KeyList>(new KeyList(dummyKey)));
            keyStore2->OnKeyGeneration(std::unique_ptr<KeyList>(new KeyList(dummyKey)));

            // retrieve key
            grpc::ServerContext ctx;
            cqp::remote::KeyRequest request;
            cqp::remote::SharedKey response;

            request.set_siteto("localhost:" + std::to_string(server2ListenPort));

            auto result = factory1.GetSharedKey(&ctx, &request, &response);
            ASSERT_TRUE(result.ok());
            ASSERT_EQ(response.keyid(), 1ul);
            ASSERT_EQ(response.keyvalue().length(), 4ul);

            KeyList keyData;
            PSK aKey;
            for(uint i = 0; i < 1000000; i++)
            {
                aKey.assign(32ul, rand());
                keyData.push_back(aKey);
                aKey.clear();
            }
            auto start = std::chrono::high_resolution_clock::now();

            keyStore1->OnKeyGeneration(std::unique_ptr<KeyList>(new KeyList(keyData)));

            auto timeTaken = std::chrono::high_resolution_clock::now() - start;
            uint timeTakenms = std::chrono::duration_cast<std::chrono::milliseconds>(timeTaken).count();
            LOGINFO("1000000 Key Storage took:" + std::to_string(timeTakenms) + "ms, " + std::to_string(timeTakenms / (1000)) + "ns per key.");

            keyStore2->OnKeyGeneration(std::unique_ptr<KeyList>(new KeyList(keyData)));

            auto start2 = std::chrono::high_resolution_clock::now();
            for(uint i = 0; i < 1000; i++)
            {
                auto result = factory1.GetSharedKey(&ctx, &request, &response);
            }
            auto timeTaken2 = std::chrono::high_resolution_clock::now() - start2;
            uint timeTakenms2 = std::chrono::duration_cast<std::chrono::milliseconds>(timeTaken2).count();
            LOGINFO("Retrieving 1000 Keys took:" + std::to_string(timeTakenms2) + "ms, " + std::to_string(timeTakenms2 / (1000)) + "ns per key.");
            server2->Shutdown();
        }
    }
}
