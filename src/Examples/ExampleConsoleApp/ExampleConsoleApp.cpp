/*!
* @file
* @brief CQP Toolkit -  QKDExampleUI
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16 Aug 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
* @example ExampleConsoleApp.cpp
*/

#include <iostream>
#include "CQPToolkit/Util/ConsoleLogger.h"
#include "CQPToolkit/Util/ProtectedVariable.h"
#include "CQPToolkit/Util/KeyVerifier.h"
#include "CQPToolkit/Util/Application.h"
#include "CQPToolkit/Util/Process.h"

#include <fstream>
#if defined(__unix__)
    #include <unistd.h>
#endif
using namespace cqp;

/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class ExampleConsoleApp : public Application, public virtual IKeyCallback
{

public:
    /// Constructor
    ExampleConsoleApp();

    ///@{
    /// @name IKeyCallback interface
    void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
    ///@}

    /// The application's main logic.
    ///
    /// Unprocessed command line arguments are passed in args.
    /// Note that all original command line arguments are available
    /// via the properties application.argc and application.argv.
    ///
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
    void displayHelp();

private:
    /// verify the keys
    KeyVerifier keyVerifier;

    /// condition to wait for
    ProtectedVariable<unsigned long> keyReceived;

    /// should the help message be displayed
    bool _helpRequested = false;
};

ExampleConsoleApp::ExampleConsoleApp()
{
    using namespace cqp;
    using std::placeholders::_1;

    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Trace);
    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&ExampleConsoleApp::HandleOption, this, _1));
}

void ExampleConsoleApp::OnKeyGeneration(std::unique_ptr<KeyList>)
{
    keyReceived.notify_one(keyReceived.GetValue() + 1);
}

void ExampleConsoleApp::HandleOption(const CommandArgs::Option& option)
{
    if (option.longName == "help")
        _helpRequested = true;
}

void ExampleConsoleApp::displayHelp()
{
    definedArguments.PrintHelp(std::cout);
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

int ExampleConsoleApp::Main(const std::vector<std::string> &args)
{
    Application::Main(args);

    if(!stopExecution)
    {
        LOGINFO("Basic application to show the possible implementation of QKD software");
    }

    return exitCode;
}

CQP_MAIN(ExampleConsoleApp)
