/*!
* @file
* @brief PublicKeyService
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "QKDInterfaces/IPublicKey.grpc.pb.h"

#include <cryptopp/oids.h>
#include <cryptopp/seckey.h>
#include <cryptopp/rsa.h>
#include <cryptopp/eccrypto.h>
#include <unordered_map>

namespace cqp
{

    /**
     * @brief The PublicKeyService class
     * Handles sharing of public keys between services.
     */
    class CQPTOOLKIT_EXPORT PublicKeyService : public remote::IPublicKey::Service
    {
    public:
        /**
         * @brief PublicKeyService
         * Constructor
         */
        PublicKeyService();

        /**
         * @brief SharePublicKey
         * Exchange public keys with another server and create a shared secret which can be used by calling GetSharedSecret
         * @param channel The server to exchange with
         * @param[in,out] token A token to identify the server pair, if left empty, it will be filled with a new token
         * @return The status of the command
         */
        grpc::Status SharePublicKey(std::shared_ptr<grpc::ChannelInterface> channel, std::string& token);

        /**
         * @brief GetSharedSecret
         * @param token The identifier for the server pair, provided by SharePublicKey
         * @return The secret as an array of bytes
         */
        std::shared_ptr<const CryptoPP::SecByteBlock> GetSharedSecret(const std::string& token) const;

        /// The name of the meta data parameter used to pass a client identifier
        static constexpr const char* TokenName = "idtoken";

        ///@{
        /// @name IPublicKey interface

        /**
         * @brief SharePublicKey
         * Called by remote PublicKeyService to complete the exchange
         * @param context
         * @param request
         * @param response
         * @return The status of the command
         */
        grpc::Status SharePublicKey(grpc::ServerContext* context, const remote::PublicKey* request, remote::PublicKey* response) override;

        ///@}
        ///
    protected:

        /// random number generator
        CryptoPP::RandomNumberGenerator* rng = nullptr;
        /// Eleiptic curve algorithm
        CryptoPP::ECDH<CryptoPP::ECP>::Domain dhA{ CryptoPP::ASN1::secp256r1() };
        /// key storage
        CryptoPP::SecByteBlock privateKey;
        /// key storage
        CryptoPP::SecByteBlock publicKey;
        /// holds a key and secret which has been agreed
        struct peerKey
        {
            /// The public key for a server
            CryptoPP::SecByteBlock key;
            /// The shared secret with that server
            std::shared_ptr<const CryptoPP::SecByteBlock> sharedSecret;
        };
        /// A lost of keys/secrets for servers which have been contacted
        std::unordered_map<std::string, peerKey> collectedKeys;

    }; // class PublicKeyService

} // namespace cqp


