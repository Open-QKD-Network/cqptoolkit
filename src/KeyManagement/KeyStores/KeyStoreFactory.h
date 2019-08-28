/*!
* @file
* @brief KeyStoreFactory
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/12/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(SQLITE3_FOUND)
#include <memory>
#include "QKDInterfaces/IKeyFactory.grpc.pb.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include <unordered_map>
#include <grpcpp/security/credentials.h>
#include "Algorithms/Datatypes/Keys.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "KeyManagement/KeyStores/FileStore.h"
#include "KeyManagement/KeyStores/IBackingStore.h"
#include <grpcpp/channel.h>

namespace cqp
{
    namespace stats
    {
        class IAllStatsCallback;
    }

    namespace keygen
    {
        class KeyStore;

        /**
         * @brief The KeyStoreFactory class
         */
        class KEYMANAGEMENT_EXPORT KeyStoreFactory : public remote::IKeyFactory::Service, public remote::IKey::Service
        {
        public:
            /**
             * @brief KeyStoreFactory
             * Constructor
             * @param clientCreds credentials used to connect to the peer
             * @param theBackingStore how to archive keys, nullptr = disabled
             */
            KeyStoreFactory(std::shared_ptr<grpc::ChannelCredentials> clientCreds, std::shared_ptr<IBackingStore> theBackingStore = nullptr);

            /**
             * @brief GetKeyStore
             * Get a keystore for a point to point link,
             * if one doesn't exist it will be created.
             * @param destination The Other end point
             * @return A Keystore
             */
            std::shared_ptr<KeyStore> GetKeyStore(const std::string& destination);

            /**
             * @brief SetSiteAddress
             * Set the site address on which this factory is running, this is needed for the
             * creation of key stores
             * @param thisSiteAddress
             */
            void SetSiteAddress(const std::string& thisSiteAddress)
            {
                siteAddress = thisSiteAddress;
            }

            ///@{
            /// @name remote::IKey interface

            /// @copydoc remote::IKey::GetKeyStores
            /// @param context Connection details from the server
            grpc::Status GetKeyStores(grpc::ServerContext* context, const google::protobuf::Empty*, remote::SiteList* response) override;

            /// @copydoc remote::IKey::GetSharedKey
            /// @param context Connection details from the server
            grpc::Status GetSharedKey(grpc::ServerContext* context, const remote::KeyRequest* request, remote::SharedKey* response) override;


            /// @}
            ///@{
            /// @name remote::IKeyFactory interface

            /// @copydoc remote::IKeyFactory::MarkKeyInUse
            /// @param context Connection details from the server
            grpc::Status MarkKeyInUse(grpc::ServerContext* context, const remote::KeyRequest* request, remote::KeyIdValue* response) override;

            /// @copydoc remote::IKeyFactory::BuildXorKey
            /// @param context Connection details from the server
            grpc::Status BuildXorKey(grpc::ServerContext* context, const remote::KeyPathRequest* request, google::protobuf::Empty*) override;

            /// @copydoc remote::IKeyFactory::GetCombinedKey
            /// @param context Connection details from the server
            grpc::Status GetCombinedKey(grpc::ServerContext* context, const remote::CombinedKeyRequest* request, remote::CombinedKeyResponse* response) override;
            /// @}

            /**
             * @brief AddReportingCallback
             * @param callback
             */
            void AddReportingCallback(stats::IAllStatsCallback* callback);
            /**
             * @brief RemoveReportingCallback
             * @param callback
             */
            void RemoveReportingCallback(stats::IAllStatsCallback* callback);

            void SetKeyStoreCacheLimit(uint64_t limit)
            {
                keyStoreCacheLimit = limit;
            }
        protected:

            /**
             * @brief GetKeystoreName
             * @param destination
             * @return  Normalised id for a keystore
             */
            std::string GetKeystoreName(const std::string& destination);

            /**
             * @brief DoCombinedKey
             * Build the command to a site to create a combined key
             * @param siteAddress
             * @param leftAddress
             * @param[in,out] leftKeyID
             * @param leftKeyIDKnown
             * @param rightAddress
             * @param rightKeyID
             * @param[out] combinedKey
             * @return Success of the command
             */
            grpc::Status DoCombinedKey(const std::string& siteAddress,
                                       const std::string& leftAddress,
                                       KeyID& leftKeyID,
                                       bool leftKeyIDKnown,
                                       const std::string& rightAddress,
                                       const KeyID& rightKeyID,
                                       std::string& combinedKey
                                      );

            /**
             * @brief GetSiteChannel
             * records know sites and creates a chennel to them
             * @param connectionAddress The address of the other site agent
             * @return A chennel to that agent
             */
            std::shared_ptr<grpc::Channel> GetSiteChannel(const std::string& connectionAddress);

        protected: // members
            /// communication channels to other site agents for creating stubs
            std::unordered_map<std::string, std::shared_ptr<grpc::Channel> > otherSites;
            /// Storage for all the created key stores
            std::unordered_map<std::string, std::shared_ptr<KeyStore>> keystores;

            /// The address which this site can be contacted on
            net::SocketAddress siteAddress;
            /// callbacks to attach to owned keystores
            std::vector<stats::IAllStatsCallback*> reportingCallbacks;
            /// credentials to use to connect to peers
            std::shared_ptr<grpc::ChannelCredentials> clientCreds;
            /// The storage to pass to the key stores
            std::shared_ptr<IBackingStore> backingStore;
            uint64_t keyStoreCacheLimit = 100000;
        };
    } // namespace keygen
} // namespace cqp
#endif // sqlite3 found
