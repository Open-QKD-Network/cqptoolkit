/*!
* @file
* @brief StatsDump
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Application.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/CommandArgs.h"
#include "QKDInterfaces/Site.pb.h"
#include "CQPToolkit/QKDDevices/LEDAliceMk1.h"
#include <grpc++/server.h>
#include "Algorithms/Random/RandomNumber.h"
#include "KeyManagement/KeyStores/KeyStore.h"
#include "QKDInterfaces/IKey.grpc.pb.h"

/**
 * @brief The SiteAgentRunner class
 * Outputs statistics in csv format
 */
class HandheldAlice : public cqp::Application, public cqp::remote::IKey::Service
{
public:
    HandheldAlice();

    /**
     * @brief DisplayHelp
     * print the help page
     */
    void DisplayHelp(const cqp::CommandArgs::Option&);

    /**
     * @brief HandleVerbose
     * Parse commandline argument
     */
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * Parse commandline argument
     */
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

protected:
    /**
     * @brief Main
     * @param args
     * @return exit code for this program
     */
    int Main(const std::vector<std::string>& args) override;

    /// credentials for making connections
    cqp::remote::Credentials creds;

    /// exit codes for this program
    enum ExitCodes { Ok = 0,
                     NoDevice = 1,
                     FailedToStartSession,
                     FailedToConnect,
                     ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

    void StopProcessing(int);
protected: // members
    std::shared_ptr<cqp::LEDAliceMk1> driver;
    std::unique_ptr<grpc::Server> keyServer;
    std::string keystoreUrl;
    std::unique_ptr<cqp::keygen::KeyStore> keystore;

    // Service interface
public:
    grpc::Status GetKeyStores(grpc::ServerContext*, const google::protobuf::Empty*, cqp::remote::SiteList* response) override;
    grpc::Status GetSharedKey(grpc::ServerContext* context, const cqp::remote::KeyRequest* request, cqp::remote::SharedKey* response) override;
};
