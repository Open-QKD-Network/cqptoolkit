/*!
* @file
* @brief Application
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Application.h"
#include "Version.h"
#include <iostream>

namespace cqp
{

    Application::Application() noexcept
    {
        using std::placeholders::_1;
        definedArguments.AddOption("version", "", "Print the version of this program")
        .Callback(std::bind(&Application::HandleVersion, this, _1));
    }

    int Application::Main(const std::vector<std::string>& args)
    {
        int result = 0;
        if(!definedArguments.Parse(args))
        {
            result = ErrorInvalidArgs;
            stopExecution = true;
        }
        return result;
    }

    int Application::main(int argc, const char* argv[])
    {
        std::vector<std::string> arguments(argv, argv + argc);
        return Main(arguments);
    }

    void Application::HandleVersion(const CommandArgs::Option& option)
    {
        std::cout << definedArguments.GetCommandName() << " Version: "
                  << TOOLKIT_VERSION_MAJOR << "."
                  << TOOLKIT_VERSION_MINOR << "."
                  << TOOLKIT_VERSION_PATCH << std::endl;
        definedArguments.StopOptionsProcessing();
        stopExecution = true;
    }

} // namespace cqp
