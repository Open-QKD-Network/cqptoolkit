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
#include "Algorithms/Logging/Logger.h"
#include <iostream>
#include <execinfo.h>
#include <unistd.h>

namespace cqp
{

    std::unordered_map<int, Application::SignalFunction> Application::signalHandlers;

    Application::Application() noexcept
    {
        using std::placeholders::_1;
        definedArguments.AddOption("version", "", "Print the version of this program")
        .Callback(std::bind(&Application::HandleVersion, this, _1));
        // add a default handler for segfaults
        std::signal(SIGSEGV, SignalHandler);
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

    void Application::HandleVersion(const CommandArgs::Option& )
    {
        std::cout << definedArguments.GetCommandName() << " Version: "
                  << TOOLKIT_VERSION_MAJOR << "."
                  << TOOLKIT_VERSION_MINOR << "."
                  << TOOLKIT_VERSION_PATCH << std::endl;
        definedArguments.StopOptionsProcessing();
        stopExecution = true;
    }

    bool Application::AddSignalHandler(int signum, SignalFunction func)
    {
        signalHandlers[signum] = func;
        bool result = std::signal(signum, SignalHandler) != SIG_ERR;
        return result;
    }

    void Application::SignalHandler(int signum)
    {
        auto handler = signalHandlers.find(signum);
        if(handler != signalHandlers.end())
        {
            try{
                handler->second(signum);
            } catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }

        if(signum == SIGSEGV)
        {
            const int maxTraces = 20;
            void *trace[maxTraces];
            int traceSize;
            traceSize = backtrace(trace, maxTraces);
            LOGERROR("SIGV Back trace:");
            backtrace_symbols_fd(trace, traceSize, STDERR_FILENO);
            exit(-1);
        }
    }

} // namespace cqp
