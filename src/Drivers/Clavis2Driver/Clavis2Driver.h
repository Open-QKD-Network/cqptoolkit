/*!
* @file
* @brief Clavis2Driver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Application.h"
#include "Algorithms/Logging/Logger.h"
#include "QKDInterfaces/Site.pb.h"
#include "QKDInterfaces/Device.pb.h"

namespace grpc
{
    class Server;
}
namespace cqp
{
    class ClavisProxy;
    class RemoteQKDDevice;
}

class Clavis2Driver : public cqp::Application
{
public:
    Clavis2Driver();

    ~Clavis2Driver() override;

    // Application interface
    int Main(const std::vector<std::string>& args) override;

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

protected: // methods

    void StopProcessing(int);

protected: // members

    /// credentials for making connections
    cqp::remote::Credentials creds;

    /// exit codes for this program
    enum ExitCodes { Ok = 0,
                     NoDevice = 1,
                     FailedToStartSession,
                     FailedToConnect,
                     ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99
                   };

    std::shared_ptr<cqp::ClavisProxy> device;
    std::unique_ptr<cqp::RemoteQKDDevice> adaptor;
    cqp::remote::ControlDetails config;
    struct CommandlineNames;
};
