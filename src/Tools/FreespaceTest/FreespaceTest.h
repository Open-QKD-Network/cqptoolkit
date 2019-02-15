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
#include "CQPToolkit/Drivers/LEDDriver.h"
#include "CQPToolkit/Drivers/UsbTagger.h"
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "Algorithms/Random/RandomNumber.h"
#include "fstream"

/**
 * @brief The SiteAgentRunner class
 * Outputs statistics in csv format
 */
class FreespaceTest : public cqp::Application,
        public virtual cqp::IDetectionEventCallback,
        public virtual cqp::IEmitterEventCallback
{
public:
    FreespaceTest();

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
     * @brief HandleQuiet
     * Parse commandline argument
     */
    void HandleConfigFile(const cqp::CommandArgs::Option& option);

protected:
    /**
     * @brief Main
     * @param args
     * @return exit code for this program
     */
    int Main(const std::vector<std::string>& args) override;

    /// exit codes for this program
    enum ExitCodes { Ok = 0,
                     NoDevice = 1,
                     ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

    void StopProcessing(int);
protected: // members
    cqp::RandomNumber rng;
    std::fstream outputFile;
    std::unique_ptr<cqp::UsbTagger> tagger;
    std::unique_ptr<cqp::LEDDriver> leds;
    std::mutex waitMutex;
    std::condition_variable waitCv;

    // IDetectionEventCallback interface
public:
    void OnPhotonReport(std::unique_ptr<cqp::ProtocolDetectionReport> report) override;

    // IEmitterEventCallback interface
public:
    void OnEmitterReport(std::unique_ptr<cqp::EmitterReport> report) override;
};
