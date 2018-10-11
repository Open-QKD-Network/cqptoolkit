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
#include "AuthUtil.h"
#include "CQPToolkit/Util/FileIO.h"
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>
#include "CQPToolkit/Util/Logger.h"
#include "grpc/grpc_security_constants.h"
#include <utility>                               // for pair, move
#include <vector>                                // for vector
#include "QKDInterfaces/Site.pb.h"               // for Credentials

namespace cqp
{

    std::shared_ptr<grpc::ChannelCredentials> LoadChannelCredentials(const remote::Credentials& creds)
    {
        using namespace std;
        std::shared_ptr<grpc::ChannelCredentials> result;

        if(creds.usetls())
        {
            grpc::SslCredentialsOptions sslOpts;
            vector<pair<const string&, string&>> params
            {
                {creds.certchainfile(), sslOpts.pem_cert_chain},
                {creds.privatekeyfile(), sslOpts.pem_private_key},
                {creds.rootcertsfile(), sslOpts.pem_root_certs},
            };

            // load the data from the files

            for(auto details : params)
            {
                if(!details.first.empty())
                {
                    if(!fs::ReadEntireFile(details.first, details.second))
                    {
                        LOGWARN("Failed to read file: " + details.first);
                    }
                }
            }

            // create the credentials
            result = grpc::SslCredentials(sslOpts);
        }
        else
        {
            LOGDEBUG("Using insecure credentials");
            result = grpc::InsecureChannelCredentials();
        }
        return result;
    }

    std::shared_ptr<grpc::ServerCredentials> LoadServerCredentials(const remote::Credentials& creds)
    {
        using namespace std;
        std::shared_ptr<grpc::ServerCredentials> result;

        if(creds.usetls())
        {
            grpc::SslServerCredentialsOptions sslOpts;
            // to allow clients to ether use no certificate or verify what they do provide
            //sslOpts.client_certificate_request = GRPC_SSL_REQUEST_CLIENT_CERTIFICATE_AND_VERIFY;
            sslOpts.client_certificate_request = GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;

            string key;
            string cert;

            if(!creds.rootcertsfile().empty())
            {
                if(!fs::ReadEntireFile(creds.rootcertsfile(), sslOpts.pem_root_certs))
                {
                    LOGERROR("Failed to read root cert file:" + creds.rootcertsfile());
                }
            }

            if(!fs::ReadEntireFile(creds.privatekeyfile(), key))
            {
                LOGERROR("Failed to read key file:" + creds.privatekeyfile());
            }
            if(!fs::ReadEntireFile(creds.certchainfile(), cert))
            {
                LOGERROR("Failed to read certificate file:" + creds.certchainfile());
            }

            sslOpts.pem_key_cert_pairs.push_back({key, cert});

            // create the credentials
            result = grpc::SslServerCredentials(sslOpts);
        }
        else
        {
            LOGDEBUG("Using insecure credentials");
            result = grpc::InsecureServerCredentials();
        }

        return result;
    }

} // namespace cqp
