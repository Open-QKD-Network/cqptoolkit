/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/DriverApplication.h"
#include "Clavis3Config.pb.h"

namespace cqp
{
    class Clavis3Device;
    namespace stats
    {
        class ReportServer;
    }
}

/**
 * @brief The Clavis3Driver class controls the IDQ clavis 3 device.
 */
class Clavis3Driver : public cqp::DriverApplication
{

    struct Names
    {
        static CONSTSTRING manual = "manual";
        static CONSTSTRING writeConfig = "write-config";
        static CONSTSTRING device = "device";
        static CONSTSTRING alice = "alice";
        static CONSTSTRING bob = "bob";
    };

public:
    /// Constructor
    Clavis3Driver();

    /// Destructor
    ~Clavis3Driver() override;

    /// The application's main logic.
    /// Returns an exit code which should be one of the values
    /// from the ExitCode enumeration.
    /// @param[in] args Unprocessed command line arguments
    /// @return success state of the application
    int Main(const std::vector<std::string>& definedArguments) override;

    /**
     * @brief HandleConfigFile
     * Parse the config file
     * @param option
     */
    void HandleConfigFile(const cqp::CommandArgs::Option& option) override;

    /**
     * @brief StopProcessing
     */
    void StopProcessing(int);

private:
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };

    std::shared_ptr<cqp::Clavis3Device> device;
    cqp::config::Clavis3Config config;

    std::shared_ptr<cqp::stats::ReportServer> reportServer;
};
