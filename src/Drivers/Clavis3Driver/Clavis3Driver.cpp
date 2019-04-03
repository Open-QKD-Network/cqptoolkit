/*!
* @file
* @brief Clavis3Driver
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Util/DriverApplication.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Clavis3Config.pb.h"
#include "IDQDevices/Clavis3/Clavis3Device.h"
#include "CQPToolkit/QKDDevices/RemoteQKDDevice.h"

using namespace cqp;

/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class Clavis3Driver : public DriverApplication
{

    struct Names
    {
        static CONSTSTRING manual = "manual";
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

    void HandleConfigFile(const cqp::CommandArgs::Option& option) override;
private:
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };

    std::shared_ptr<cqp::Clavis3Device> device;
    cqp::config::Clavis3Config config;
};

Clavis3Driver::Clavis3Driver()
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    definedArguments.AddOption(Names::manual, "m", "Manual mode, specify Bobs address to directly connect and start generating key")
    .Bind();

}

Clavis3Driver::~Clavis3Driver()
{
    adaptor.reset();
    device.reset();
}

void Clavis3Driver::HandleConfigFile(const cqp::CommandArgs::Option& option)
{
    ParseConfigFile(option, config);
}

int Clavis3Driver::Main(const std::vector<std::string> &args)
{
    Application::Main(args);

    if(!stopExecution)
    {
        using namespace std;
    }

    return 0;
}

CQP_MAIN(Clavis3Driver)
