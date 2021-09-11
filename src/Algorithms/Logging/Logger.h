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
#pragma once
#include <exception>
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Logging/ILogger.h"
#include <string>
#include <unordered_map>
#include <vector>

#if !defined(CQP_LOGGER)
/// Allow definition of the logger to be overridden
/// If a different default logger is required, define this symbol
#define CQP_LOGGER
namespace cqp
{

#if defined(_MSC_VER)
    /// Provide a common way to access the name of the current function
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#endif // defined(_MSC_VER)

#if defined(_WIN32)
    #define LOGLASTERROR cqp::DefaultLogger().Log(cqp::LogLevel::Error, __FUNCTION__ + std::string(_com_error(GetLastError()).ErrorMessage()));
#endif // defined(_WIN32)

    ///standard macro for reporting unimplemented functions
#define CQP_UNIMPLEMENTED cqp::DefaultLogger().Log(cqp::LogLevel::Debug, "Function unimplemented");
#if defined(_DEBUG)
    /// Output a trace message
    #define LOGTRACE(x) cqp::DefaultLogger().Log(cqp::LogLevel::Trace, std::string(__FILE__) + "." + std::to_string(__LINE__) + ":" + std::string(__FUNCTION__) + ": " + x)
    /// Output a debug message
    #define LOGDEBUG(x) cqp::DefaultLogger().Log(cqp::LogLevel::Debug, std::string(__FILE__) + "." + std::to_string(__LINE__) + ":" + std::string(__FUNCTION__) + ": " + x)

    /// Output an informational message
    #define LOGINFO(x) cqp::DefaultLogger().Log(cqp::LogLevel::Info, std::string(__FILE__) + "." + std::to_string(__LINE__) + ":" + std::string(__FUNCTION__) + ": " + x)
    /// Output a warning message
    #define LOGWARN(x) cqp::DefaultLogger().Log(cqp::LogLevel::Warning, std::string(__FILE__) + "." + std::to_string(__LINE__) + ":" + std::string(__FUNCTION__) + ": " + x)
    /// Output a warning message
    #define LOGERROR(x) cqp::DefaultLogger().Log(cqp::LogLevel::Error, std::string(__FILE__) + "." + std::to_string(__LINE__) + ":" + std::string(__FUNCTION__) + ": " + x)

#else
    /// Trace messages are disabled in this build
    #define LOGTRACE(x)
    /// Debug messages are disabled in this build
    #define LOGDEBUG(x)

    /// Output an informational message
    #define LOGINFO(x) cqp::DefaultLogger().Log(cqp::LogLevel::Info, std::string("") + x)
    /// Output a warning message
    #define LOGWARN(x) cqp::DefaultLogger().Log(cqp::LogLevel::Warning, std::string("") + x)
    /// Output a warning message
    #define LOGERROR(x) cqp::DefaultLogger().Log(cqp::LogLevel::Error, std::string("") + x)

#endif
#if (__GNUC__ < 7) && !defined(__clang__) && !defined(WIN32)
    /// Used for producing standard prefixs to levels
    using LevelMap = std::unordered_map<LogLevel, std::string, EnumClassHash>;
#else
    /// Used for producing standard prefixes to levels
    using LevelMap = std::unordered_map<LogLevel, std::string>;
#endif

    /// Used for producing standard prefixes to levels
    const static LevelMap LEVELPREFIX =
    {
        {LogLevel::Debug, "DEBUG: "},
        {LogLevel::Error, "ERROR: "},
        {LogLevel::Info, "INFO: "},
        {LogLevel::Trace, "TRACE: "},
        {LogLevel::Warning, "WARN: "}
    };

    /// Standardised interface for logging used by the toolkit.
    class ALGORITHMS_EXPORT Logger : public virtual ILogger
    {
    public:

		static std::string GetLastErrorString();

        static std::string GetTimeStamp();

        /// Change the level out output from the logger
        /// @param[in] level Message which are as or more severe as this will be printed
        void SetOutputLevel(LogLevel level) override;
        /// Gets the current setting of the filter for logging
        /// @return The currently limited log level
        LogLevel GetOutputLevel() const  override
        {
            return currentOutput;
        }

        /// Default destructor
        ~Logger() override = default;

        /// Send output to the logger
        /// @param[in] level message severity
        /// @param[in] message The Message to display
        void Log(LogLevel level, const std::string& message) override
        {
            // Pass the message on to any loggers attached to us
            for (ILogger* childLogger : subLoggers)
            {
                childLogger->Log(level, message);
            }
        }

        /// Daisy chain another logger so that it will receive the same messages as the top level logger.
        /// @param newLogger The logger to attach
        void AttachLogger(ILogger* const newLogger) override;

        /// Remove a logger from the chain.
        /// @param logger The logger to remove
        void DettachLogger(ILogger* const logger) override;

        /// Increase the level of output
        void IncOutputLevel() override;

        /// Decrease the level of output
        void DecOutputLevel() override;

        /// default constructor
        Logger() = default;
    protected:

        /// The level at which messages will be printed.
        LogLevel currentOutput = LogLevel::Warning;

        /// A lost of any loggers attached to this logger so that messages can be handled by multiple loggers
        std::vector<ILogger*> subLoggers;
    };

    /// A function definition for retrieving the default logger.
    /// @remarks this currently allows the linker to be defined at link time,
    ///     this may be flexible enough by requires understanding that something must implement
    ///     this function for the link to be a success.
    /// @return The default logger
    ALGORITHMS_EXPORT ILogger& DefaultLogger();
}
#endif
