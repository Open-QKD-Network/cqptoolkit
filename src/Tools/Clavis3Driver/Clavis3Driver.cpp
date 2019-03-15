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

#include <iostream>
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/Application.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Util/Strings.h"
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include <google/protobuf/util/json_util.h>

#include <thread>
#include "IDQDevices/Clavis3/Clavis3Device.h"

using namespace cqp;


/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class Clavis3Driver : public Application
{

    struct Names
    {
        static CONSTSTRING configFile = "config-file";
        static CONSTSTRING id = "id";
        static CONSTSTRING port = "port";
        static CONSTSTRING certFile = "cert";
        static CONSTSTRING keyFile = "key";
        static CONSTSTRING rootCaFile = "rootca";
        static CONSTSTRING tls = "tls";
        static CONSTSTRING connect = "connect";
    };

public:
    /// Constructor
    Clavis3Driver();

    /// Destructor
    ~Clavis3Driver() override = default;

    /// The application's main logic.
    /// Returns an exit code which should be one of the values
    /// from the ExitCode enumeration.
    /// @param[in] args Unprocessed command line arguments
    /// @return success state of the application
    int Main(const std::vector<std::string>& definedArguments) override;

protected:
    /// Called when the option with the given name is encountered
    /// during command line arguments processing.
    ///
    /// The default implementation does option validation, bindings
    /// and callback handling.
    /// @param[in] name Option name
    /// @param[in] value Option Value
    void HandleOption(const CommandArgs::Option& option);

    /// print the help page
    void DisplayHelp(const CommandArgs::Option& option);

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

private:
    std::shared_ptr<grpc::ChannelCredentials> clientCreds = grpc::InsecureChannelCredentials();

    /// should the help message be displayed
    bool _helpRequested = false;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };

    std::unique_ptr<Clavis3Device> device;
};

Clavis3Driver::Clavis3Driver()
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    definedArguments.AddOption(Names::configFile, "c", "load configuration data from a file")
    .Bind();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&Clavis3Driver::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::id, "i", "Site Agent ID")
    .Bind();

    definedArguments.AddOption(Names::port, "p", "Listen on this port")
    .Bind();

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&Clavis3Driver::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::connect, "r", "Connect to other site")
    .Bind();

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&Clavis3Driver::HandleVerbose, this, _1));


    // Site agent drivers
    //DummyQKD::RegisterWithFactory();
    // ^^^^^^ Add new drivers here ^^^^^^^^^

}

void Clavis3Driver::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;
    string driverNames;

    definedArguments.PrintHelp(std::cout, "Creates CQP Site Agents for managing QKD systems.\nCopyright Bristol University. All rights reserved.",
                               "Drivers: " + driverNames);
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
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
