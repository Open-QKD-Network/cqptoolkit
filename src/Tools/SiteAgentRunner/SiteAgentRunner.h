/*!
* @file
* @brief SiteAgentRunner
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPAlgorithms/Util/Application.h"
#include "CQPAlgorithms/Util/CommandArgs.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include "CQPToolkit/KeyGen/SiteAgent.h"
#include "CQPToolkit/SDN/NetworkManagerDummy.h"

/**
 * @brief The SiteAgentRunner class
 * Starts a local SiteAgent
 */
class SiteAgentRunner : public cqp::Application
{
public:
    SiteAgentRunner();

    /**
     * @brief HandleDevice
     * @param option
     */
    void HandleDevice(const cqp::CommandArgs::Option& option);

    /// print the help page
    void DisplayHelp(const cqp::CommandArgs::Option&);

    /**
     * @brief HandleVerbose
     * Make the program more verbose
     */
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * Make the program quieter
     */
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

    /**
     * @brief RunTests
     * @param siteSettings
     */
    void RunTests(cqp::remote::SiteAgentConfig& siteSettings);

protected:
    /**
     * @brief Main
     * @param args
     * @return program exit code
     */
    int Main(const std::vector<std::string>& args) override;

    /// agents managed by this site
    std::vector<cqp::SiteAgent*> siteAgents;
    /// devices connected to this site
    std::vector<std::string> deviceAddresses;
    /// for detecting other sites
    std::unique_ptr<cqp::net::ServiceDiscovery> sd;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };
};
