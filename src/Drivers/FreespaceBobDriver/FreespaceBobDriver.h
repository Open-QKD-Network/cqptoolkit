/*!
* @file
* @brief FreespaceBobDriver
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/DriverApplication.h"
#include "Algorithms/Logging/Logger.h"
#include <memory>
#include "FreespaceConfig.pb.h"
#include "QKDInterfaces/Site.pb.h"

namespace grpc
{
    class Server;
}
namespace cqp
{
    class PhotonDetectorMk1;
    class RemoteQKDDevice;
}

class FreespaceBobDriver : public cqp::DriverApplication
{
public:
    FreespaceBobDriver();

    ~FreespaceBobDriver() override;

    // Application interface
    int Main(const std::vector<std::string>& args) override;

    /**
     * @brief HandleQuiet
     * Parse commandline argument
     */
    void HandleConfigFile(const cqp::CommandArgs::Option& option) override;

protected: // methods

    void StopProcessing(int);

protected: // members

    /// exit codes for this program
    enum ExitCodes { Ok = 0,
                     NoDevice = 1,
                     FailedToStartSession,
                     FailedToConnect,
                     ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99
                   };

    std::shared_ptr<cqp::PhotonDetectorMk1> device;
    cqp::config::FreespaceConfig config;
    struct FreespaceNames;
};
