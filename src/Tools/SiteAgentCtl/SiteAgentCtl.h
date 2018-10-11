/*!
* @file
* @brief SiteAgentCtl
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
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include "CQPToolkit/KeyGen/SiteAgent.h"

/**
 * @brief The SiteAgentCtl class
 * Send commands to site agents
 */
class SiteAgentCtl : public cqp::Application
{
public:
    /**
     * @brief SiteAgentCtl
     * Constructor
     */
    SiteAgentCtl();
    /**
     * @brief HandleHelp
     * Print the help message
     * @param option details of the option being parsed
     */
    void DisplayHelp(const cqp::CommandArgs::Option& option);

    /**
     * @brief HandleStart
     * @param option
     */
    void HandleStart(const cqp::CommandArgs::Option& option);
    /**
     * @brief HandleStop
     * @param option
     */
    void HandleStop(const cqp::CommandArgs::Option& option);
    /**
     * @brief HandleStop
     * @param option
     */
    void HandleDetails(const cqp::CommandArgs::Option& option);
    /**
     * @brief HandleListSites
     * @param option
     */
    void HandleListSites(const cqp::CommandArgs::Option& option);

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

    void HandleGetKey(const cqp::CommandArgs::Option& option);

    /**
     * @brief ListSites
     * Output a list of known sites for the connected site
     * @param siteA
     */
    void ListSites(cqp::remote::IKey::Stub* siteA);
    /**
     * @brief StartNode
     * Start two or more site agents
     * @param siteA
     * @param path
     */
    void StartNode(cqp::remote::ISiteAgent::Stub* siteA, const cqp::remote::PhysicalPath& path);
    /**
     * @brief StopNode
     * Stop agents from exchanging keys
     * @param siteA
     * @param path
     */
    void StopNode(cqp::remote::ISiteAgent::Stub* siteA, const cqp::remote::PhysicalPath& path);

    /**
     * @brief ListSites
     * Output a list of known sites for the connected site
     * @param siteA
     */
    void GetDetails(cqp::remote::ISiteAgent::Stub* siteA);

    /**
     * @brief GetKey
     * Get a key from the keystore
     * @param siteA
     * @param destination
     */
    void GetKey(cqp::remote::IKey::Stub* siteA, const std::string& destination);
protected:
    /**
     * @brief Main
     * Main entry
     * @param args
     * @return program error code
     */
    int Main(const std::vector<std::string>& args) override;

    /**
     * @brief The Command struct
     */
    struct Command
    {
        /// command types
        enum class Cmd { Start, Stop, List, Details, Key };

        /**
         * @brief Command
         * Constructor
         * @param type
         */
        Command(Cmd type) : cmd(type)
        {
        }

        /// path for key to take
        cqp::remote::PhysicalPath physicalPath;
        /// Destination for key request
        std::string destination;
        /// which command to run
        const Cmd cmd;
    };

    /// commands parsed from the command line
    std::vector<Command> commands;
    /// credentials to use when connecting
    cqp::remote::Credentials creds;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };
};
