/*!
* @file
* @brief %{Cpp:License:ClassName}
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
#include <random>
#include "CQPToolkit/Statistics/ReportServer.h"
#include "CQPToolkit/Statistics/StatisticsLogger.h"
#include "QKDInterfaces/Site.pb.h"

/**
 * @brief The SiteAgentRunner class
 * Outputs statistics in csv format
 */
class StatsGenerator : public cqp::Application
{
public:
    StatsGenerator();

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

    /**
     * @brief HandleVerbose
     * Parse commandline argument
     * @param option
     */
    void HandleType(const cqp::CommandArgs::Option& option);

protected:
    /**
     * @brief Main
     * @param args
     * @return exit code for this program
     */
    int Main(const std::vector<std::string>& args) override;

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

    /// for detecting services
    std::unique_ptr<cqp::net::ServiceDiscovery> sd;
    /// credentials for making connections
    cqp::remote::Credentials creds;

    /// our server, other interfaces will hang off of this
    std::unique_ptr<grpc::Server> server;
    /// known services
    std::vector<std::string> serviceUrls;
    /// active connections
    std::map<std::string, ServiceConnection> connections;
    /// the possible types of messages
    std::map<std::string, bool> allMessageTypes;

    /// Distribution algorithms to ensure good distribution of numbers
    std::uniform_int_distribution<int> smallIntDistribution;
    /// Distribution algorithms to ensure good distribution of numbers
    std::uniform_real_distribution<double> percentDistribution;
    /// Distribution algorithms to ensure good distribution of numbers
    std::uniform_int_distribution<int> tinyIntDistribution;
    /// Distribution algorithms to ensure good distribution of numbers
    std::uniform_int_distribution<int> updateDistribution;

    /// Random number generator
    std::default_random_engine generator;

    /// for storing reports
    cqp::stats::ReportServer reportServer;
    /// pro printing reports
    cqp::stats::StatisticsLogger statsLogger;

    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

};
