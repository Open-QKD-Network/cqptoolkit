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
#include "Algorithms/Util/Application.h"
#include "Algorithms/Util/CommandArgs.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/Random/RandomNumber.h"

/**
 * @brief The SiteAgentRunner class
 * Outputs statistics in csv format
 */
class QKDPostProc : public cqp::Application
{
public:
    QKDPostProc();

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

    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

protected: // members
    /// The random number generator
    std::shared_ptr<cqp::RandomNumber> rng{new cqp::RandomNumber()};
};
