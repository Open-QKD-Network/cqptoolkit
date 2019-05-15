/*!
* @file
* @brief CQP Toolkit - Logging
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Logger.h"

namespace cqp
{

    /// The logger which will be used to propagate all log messages to other loggers and to users
    Logger defaultLogger;

    /// Change the level out output from the logger
    /// @param[in] level Message which are as or more severe as this will be printed
    void Logger::SetOutputLevel(LogLevel level)
    {
        currentOutput = level;
        // Pass the change on to any loggers attached to us
        for (ILogger* childLogger : subLoggers)
        {
            childLogger->SetOutputLevel(level);
        }
    }

    /// Daisy chain another logger so that it will receive the same messages as the top level logger.
    void Logger::AttachLogger(ILogger* newLogger)
    {
        subLoggers.push_back(newLogger);
        newLogger->SetOutputLevel(currentOutput);
    }

    /// Daisy chain another logger so that it will receive the same messages as the top level logger.
    void Logger::DettachLogger(ILogger* logger)
    {
        for (unsigned long index = 0; index < subLoggers.size(); index++)
        {
            if (subLoggers[index] == logger)
            {
                subLoggers.erase(subLoggers.begin() + static_cast<long>(index));
                break;
            }
        }
    }

    void Logger::IncOutputLevel()
    {
        switch(currentOutput)
        {
        case LogLevel::Silent:
            currentOutput = LogLevel::Error;
            break;
        case LogLevel::Error:
            currentOutput = LogLevel::Warning;
            break;
        case LogLevel::Warning:
            currentOutput = LogLevel::Info;
            break;
        case LogLevel::Info:
            currentOutput = LogLevel::Debug;
            break;
        case LogLevel::Debug:
            currentOutput = LogLevel::Trace;
            break;
        case LogLevel::Trace:
            break;
        }

        SetOutputLevel(currentOutput);
    }

    void Logger::DecOutputLevel()
    {
        switch(currentOutput)
        {
        case LogLevel::Silent:
            break;
        case LogLevel::Error:
            currentOutput = LogLevel::Silent;
            break;
        case LogLevel::Warning:
            currentOutput = LogLevel::Error;
            break;
        case LogLevel::Info:
            currentOutput = LogLevel::Warning;
            break;
        case LogLevel::Debug:
            currentOutput = LogLevel::Info;
            break;
        case LogLevel::Trace:
            currentOutput = LogLevel::Debug;
            break;
        }

        SetOutputLevel(currentOutput);
    }

    /// A function definition for retrieving the default logger.
    /// @remarks this currently allows the linker to be defined at link time,
    ///     this may be flexible enough by requires understanding that something must implement
    ///     this function for the link to be a success.
    /// @return The default logger
    ILogger& DefaultLogger()
    {
        return defaultLogger;
    }

}
