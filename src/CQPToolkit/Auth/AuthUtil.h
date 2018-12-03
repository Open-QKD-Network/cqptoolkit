/*!
* @file
* @brief AuthUtil
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <memory>                    // for allocator
#include <stdlib.h>                  // for getenv, setenv
#include <string>                    // for operator+, allocator, char_traits
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/cqptoolkit_export.h"

namespace grpc
{
    class ChannelCredentials;
    class ServerCredentials;
}

namespace cqp
{
    namespace remote
    {
        class Credentials;
    }

    /// environment variable name for controlling
    /// which cipher suites grpc will use
    constexpr const char* GRPC_SSL_CIPHER_SUITES = "GRPC_SSL_CIPHER_SUITES";
    /// The cipher suites used if GrpcAllowMACOnlyCiphers is called
    constexpr const char* SupportedCiphers =
        // Non-encrypting, authenticated schemes
        // TODO: grpc wont let us use these yet as it requires an encryption scheme
        "DHE-PSK-NULL-SHA256"
        ":ECDHE-PSK-NULL-SHA256"
        ":DHE-PSK-NULL-SHA384"
        ":ECDHE-PSK-NULL-SHA384"
        // ECDSA encrypting scheme
        ":ECDHE-ECDSA-AES128-GCM-SHA256"
        ":ECDHE-ECDSA-AES256-GCM-SHA384"
        // default schemes
        ":ECDHE-RSA-AES128-GCM-SHA256"
        ":ECDHE-RSA-AES256-GCM-SHA384"
        ;

    /**
     * @brief GrpcAllowMACOnlyCiphers
     * Perform GRPC environment setup. This must be called before any grpc interface.
     * By allowing "null" encryption schemes, the messages will be sent unencrypted (improving performance)
     * but ensuring that messages are still authenticated.
     */
    inline void GrpcAllowMACOnlyCiphers()
    {
        if(getenv(GRPC_SSL_CIPHER_SUITES) == nullptr) /* Flawfinder: ignore */
        {
            // the env var has not been set externally
            LOGDEBUG("Setting GRPC_SSL_CIPHER_SUITES to " + SupportedCiphers);
            // Before the gRPC native library (gRPC Core) is lazily loaded and
            // initialized, an environment variable must be set
            setenv(GRPC_SSL_CIPHER_SUITES, SupportedCiphers, 1);
        }
    }

    /**
     * @brief LoadChannelCredentials
     * Create a set of credentials based on settings
     * @param creds settings
     * @return Credentials for use with connecting to servers
     */
    CQPTOOLKIT_EXPORT std::shared_ptr<grpc::ChannelCredentials> LoadChannelCredentials(const remote::Credentials& creds);

    /**
     * @brief LoadServerCredentials
     * Create a set of credentials based on settings
     * @param creds settings
     * @return Credentials for starting a server
     */
    CQPTOOLKIT_EXPORT std::shared_ptr<grpc::ServerCredentials> LoadServerCredentials(const remote::Credentials& creds);

} // namespace cqp


