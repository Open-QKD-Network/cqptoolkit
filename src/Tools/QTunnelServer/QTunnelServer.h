/*!
* @file
* @brief QTunnelServer
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
#include "CQPToolkit/Util/CommandArgs.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "CQPToolkit/Tunnels/Controller.h"
#include "CQPToolkit/Statistics/ReportServer.h"

/**
 * @brief The QTunnelServer class
 * A program for controlling encrypted tunnels which use the cqp::remote::IKeyFactory interface to get pre-shared keys
 */
class QTunnelServer : public cqp::Application
{
public:
    /// Constructor
    QTunnelServer();

    /**
     * @brief main
     * Entry point for the local agent which creates tunnel
     * @param args
     * @return Program return code
     */
    int Main(const std::vector<std::string>& args) override;

    /**
     * @brief LoadConfig
     * Configure the system based on the configuration file
     * @return true on success
     */
    bool LoadConfig();
    /**
     * @brief HandleHelp
     * Print the help message
     * @param option details of the option being parsed
     */
    void HandleHelp(const cqp::CommandArgs::Option& option);

    /**
     * @brief HandleVerbose
     * Make the system more verbose
     * @param option details of the option being parsed
     */
    void HandleVerbose(const cqp::CommandArgs::Option& option)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * make the system quieter
     * @param option details of the option being parsed
     */
    void HandleQuiet(const cqp::CommandArgs::Option& option)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

protected:
    /// exit codes for the program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };

    /// Detection of running services
    std::unique_ptr<cqp::net::ServiceDiscovery> sd;
    /// service for peers to connect to
    std::unique_ptr<grpc::Server> server;
    /// the port number which the server is listening on
    int listenPort = 0;

    /// settings for this program
    cqp::remote::tunnels::ControllerDetails controllerSettings;
    /// The controllers managed by this program
    std::shared_ptr<cqp::tunnels::Controller> controller;
    /// for statistic publishing
    cqp::stats::ReportServer reportServer;
}; // class QTunnelServer
