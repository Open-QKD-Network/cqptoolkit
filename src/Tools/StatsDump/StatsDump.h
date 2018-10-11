/*!
* @file
* @brief StatsDump
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Application.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "CQPToolkit/Util/CommandArgs.h"
#include <grpcpp/channel.h>
#include "QKDInterfaces/IReporting.grpc.pb.h"
#include "QKDInterfaces/Site.pb.h"

/**
 * @brief The SiteAgentRunner class
 * Outputs statistics in csv format
 */
class StatsDump : public cqp::Application, public cqp::IServiceCallback
{
public:
    StatsDump();

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

    /**
     * @brief HandleVerbose
     * Parse commandline argument
     * @param option
     */
    void HandleService(const cqp::CommandArgs::Option& option)
    {
        serviceUrls.push_back(option.value);
    }

    //@{
    /// IServiceCallback interface

    /**
     * @brief OnServiceDetected
     * @param newServices
     * @param deletedServices
     */
    void OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices) override;

    ///@}
protected:
    /**
     * @brief Main
     * @param args
     * @return exit code for this program
     */
    int Main(const std::vector<std::string>& args) override;

    /**
     * @brief CollectStatsFrom
     * @param address
     */
    void CollectStatsFrom(const std::string& address);

    /// A connection
    struct ServiceConnection
    {
        /// the identifier for the connection
        std::string name;
        /// channel connection
        std::shared_ptr<grpc::Channel> channel;
        /// the thread reading the stats
        std::unique_ptr<std::thread> task;
    };

    /**
     * @brief ReadStats
     * @param from
     * @param stub
     */
    void ReadStats(std::string from, std::unique_ptr<cqp::remote::IReporting::Stub> stub);

    /// for detecting services
    std::unique_ptr<cqp::net::ServiceDiscovery> sd;
    /// credentials for making connections
    cqp::remote::Credentials creds;

    /// known services
    std::vector<std::string> serviceUrls;
    /// active connections
    std::map<std::string, ServiceConnection> connections;
    /// filter to specify when connecting
    cqp::remote::ReportingFilter defaultFilter;

    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

    /// ensures output is contiguous
    std::mutex outputLock;
};
