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
#include "Algorithms/Util/Application.h"
#include "Algorithms/Util/CommandArgs.h"
#include "Algorithms/Logging/Logger.h"
#include "KeyManagement/Sites/SiteAgent.h"
#include "KeyManagement/SDN/NetworkManagerDummy.h"

/**
 * @brief The SiteAgentRunner class
 * Starts a local SiteAgent
 */
class SiteAgentRunner : public cqp::Application
{
public:
    SiteAgentRunner();

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

protected:
    /**
     * @brief Main
     * @param args
     * @return program exit code
     */
    int Main(const std::vector<std::string>& args) override;

    void StopProcessing(int);

    /// agents managed by this site
    std::vector<std::unique_ptr<cqp::SiteAgent>> siteAgents;
    /// for detecting other sites
    std::unique_ptr<cqp::net::ServiceDiscovery> sd;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };
};
