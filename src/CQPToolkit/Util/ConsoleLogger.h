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
#pragma once
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Util/Platform.h"
#include <mutex>

namespace cqp
{
#if defined(_WIN32)

    /// Creates a structure to map an ansi colour escape sequence
    /// @returns the prebuilt array
    static LevelMap CreateLevelToColour()
    {
        LevelMap result;

        result[LogLevel::Debug] = "";
        result[LogLevel::Error] = "";
        result[LogLevel::Info] = "";
        result[LogLevel::Trace] = "";
        result[LogLevel::Warning] = "";
        return result;
    }
#else
    /// Creates a structure to map an ansi colour escape sequence
    /// @returns the prebuilt array
    static LevelMap CreateLevelToColour()
    {
        LevelMap result;

        result[LogLevel::Debug] ="\x1b[34;47m";
        result[LogLevel::Error] = "\x1b[31m";
        result[LogLevel::Info] = "\x1b[32m";
        result[LogLevel::Trace] = "\x1b[37m";
        result[LogLevel::Warning] = "\x1b[93;41m";
        return result;
    }
#endif

    /// Maps a log level to an ansi colour escape sequence
    const static LevelMap levelToColour = CreateLevelToColour();

    /// @brief A Standard output logger for use with command line output.
    class CQPTOOLKIT_EXPORT ConsoleLogger :
        public Logger
    {
    public:
        /// @brief Output a message at a given severity level
        /// @note The current output level may hide the message.
        /// @param[in] level The severity of the log message
        /// @param[in] message The message to display
        virtual void Log(LogLevel level, const std::string& message) override;
        /// Standard distructor
        virtual ~ConsoleLogger() override;
        /// Start using the console logger
        /// Calling this when already enabled will have no effect
        static void Enable();
    protected:

        /// Default constructor which attaches itself as a logger
        ConsoleLogger();

    private:
        /// Prevent multiple threads from spamming the output
        std::mutex consoleLock;
        /// whether this system can print text in colour
        bool supportsColour = false;
    };

}
