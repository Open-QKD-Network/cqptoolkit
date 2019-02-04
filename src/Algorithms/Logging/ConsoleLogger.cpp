/*!
* @file
* @brief CQP Toolkit - Console Logger
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Algorithms/Logging/ConsoleLogger.h"
#include <iostream>
#include <string>
#if defined(__unix)
    #include <unistd.h>
    #include <cstdio>
#endif

namespace cqp
{
    using namespace std;
    /// Holds the single instance of the console logger
    ConsoleLogger* ConsoleLogger::theConsoleLogger = nullptr;

    ConsoleLogger::ConsoleLogger()
    {
        // Attach ourselves as a logger
        DefaultLogger().AttachLogger(this);

#if defined(WIN32)
        supportsColour = true;
#elif defined(__unix)
        supportsColour = isatty(fileno(stderr));
#endif

    }

    void ConsoleLogger::Log(LogLevel level, const std::string& message)
    {
        /// Ansi escape sequence for the default console colour
        static const std::string DefaultColour = "\x1b[39;49m";

        if (level > LogLevel::Silent && level <= currentOutput)
        {
            lock_guard<mutex> lock(consoleLock);
            if(supportsColour)
            {
                std::cerr << levelToColour.at(level);
            }

            std::cerr << LEVELPREFIX.at(level) << message;

            if(supportsColour)
            {
                std::cerr << DefaultColour;
            }
            std::cerr << endl << flush;
        }

        // Up call to parent class
        Logger::Log(level, message);

    }

    ConsoleLogger::~ConsoleLogger()
    {
        DefaultLogger().DettachLogger(this);
    }

    void ConsoleLogger::Enable()
    {
        if (theConsoleLogger == nullptr)
        {
            theConsoleLogger = new ConsoleLogger();
        }
    }

}
