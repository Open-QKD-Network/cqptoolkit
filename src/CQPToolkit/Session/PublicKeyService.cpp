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
#include "PublicKeyService.h"

#if defined(CRYPTOPP_HASRDRAND)
    #include <cryptopp/rdrand.h>
#endif
#include "CQPToolkit/Util/GrpcLogger.h"
#include <cryptopp/osrng.h>
#include "CQPAlgorithms/Datatypes/UUID.h"
namespace cqp
{

    PublicKeyService::PublicKeyService()
    {
        using namespace CryptoPP;
        // create space for the keys
        privateKey.CleanNew(dhA.PrivateKeyLength());
        publicKey.CleanNew(dhA.PublicKeyLength());

        rng = new CryptoPP::AutoSeededX917RNG<AES>();
        // generate our private/public keys
        dhA.GenerateKeyPair(*rng, privateKey, publicKey);
    }

    grpc::Status PublicKeyService::SharePublicKey(std::shared_ptr<grpc::ChannelInterface> channel, std::string& token)
    {
        using namespace std;
        using namespace grpc;
        grpc::Status result;
        // connect to our peer and request their key

        unique_ptr<remote::IPublicKey::Stub> peer = remote::IPublicKey::NewStub(channel);
        if(peer != nullptr)
        {
            ClientContext ctx;
            remote::PublicKey request;
            remote::PublicKey response;
            LOGTRACE("Sending our public key");
            (*request.mutable_format()) = dhA.AlgorithmName();
            request.mutable_keyvalue()->assign(publicKey.begin(), publicKey.end());

            if(!token.empty())
            {
                // if we already have a token, send it in the metadata
                ctx.AddMetadata(TokenName, token);
            }

            // request the peers public key
            result = LogStatus(
                         peer->SharePublicKey(&ctx, request, &response));
            // the token for this pair of keys will be in the metadata
            auto trailingMetadata = ctx.GetServerTrailingMetadata();
            auto tokenIt = trailingMetadata.find(TokenName);

            if(tokenIt == trailingMetadata.end())
            {
                result = Status(StatusCode::INVALID_ARGUMENT, "Invalid metadata");
            }

            if(result.ok())
            {
                LOGTRACE("Peer token received");
                token = string(tokenIt->second.begin(), tokenIt->second.end());
                collectedKeys[token].key.CleanNew(response.keyvalue().size());
                collectedKeys[token].key.Assign(
                    reinterpret_cast<const unsigned char*>(response.keyvalue().data()), response.keyvalue().size());

                std::shared_ptr<CryptoPP::SecByteBlock> newSecret(new CryptoPP::SecByteBlock(dhA.AgreedValueLength()));

                if(dhA.Agree(*newSecret, privateKey, collectedKeys[token].key))
                {
                    // store the secret which has been calculated
                    collectedKeys[token].sharedSecret = newSecret;
                    LOGTRACE("Shared secret created");
                } // if keys agree
                else
                {
                    result = Status(grpc::StatusCode::INVALID_ARGUMENT, "Could not generate agreed shared secret");
                } // else
            } //  if(result.ok())
        } // if peer != null
        else
        {
            result = LogStatus(Status(StatusCode::UNAVAILABLE, "Channel does not provide IPublicKey"));
        } // else
        return result;
    } // SharePublicKey

    grpc::Status PublicKeyService::SharePublicKey(grpc::ServerContext* context, const remote::PublicKey* request, remote::PublicKey* response)
    {
        using grpc::Status;
        Status result;
        std::string token;
        LOGTRACE("Our key has been requested");

        auto clientTokenIt = context->client_metadata().find("");
        if(clientTokenIt == context->client_metadata().end())
        {
            // no token was provided, create a new one
            LOGTRACE("Creating new token");
            token = UUID().ToString();
            context->AddTrailingMetadata(TokenName, token);
        } // token exists
        else
        {
            token = std::string(clientTokenIt->second.begin(), clientTokenIt->second.end());
        } // else

        // make sure we're talking the same language
        if(dhA.AlgorithmName() == request->format())
        {
            // copy their public key
            collectedKeys[token].key.CleanNew(request->keyvalue().size());
            collectedKeys[token].key.Assign(
                reinterpret_cast<const unsigned char*>(request->keyvalue().data()), request->keyvalue().size());
            (*response->mutable_format()) = dhA.AlgorithmName();
            // copy our public key into the output
            response->mutable_keyvalue()->assign(publicKey.begin(), publicKey.end());

            std::shared_ptr<CryptoPP::SecByteBlock> newSecret(new CryptoPP::SecByteBlock(dhA.AgreedValueLength()));

            // agree on a secret
            if(dhA.Agree(*newSecret, privateKey, collectedKeys[token].key))
            {
                // save it for later
                collectedKeys[token].sharedSecret = newSecret;
                LOGTRACE("Shared secret created");
            } // if agree
            else
            {
                result = LogStatus(Status(grpc::StatusCode::INVALID_ARGUMENT, "Could not generate agreed shared secret"));
            } // else
        } // if formats match
        else
        {
            result = LogStatus(Status(grpc::StatusCode::INVALID_ARGUMENT, "Key algorithm not supported"));
        } // else

        return result;
    } // SharePublicKey

    std::shared_ptr<const CryptoPP::SecByteBlock> PublicKeyService::GetSharedSecret(const std::string& token) const
    {
        std::shared_ptr<const CryptoPP::SecByteBlock> result;
        auto it = collectedKeys.find(token);
        if(it != collectedKeys.end())
        {
            result = it->second.sharedSecret;
        } // key exists
        else
        {
            LOGERROR("Unknown key token: " + token);
        }

        return result;
    } // GetSharedSecret

} // namespace cqp
