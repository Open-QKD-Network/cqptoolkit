/*!
* @file
* @brief CQP Toolkit - Logging Interface
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Platform.h"
#include <string>

namespace cqp
{
    /// The severity of the message
    enum class LogLevel
    {
        Silent = 0, Error, Warning, Info, Debug, Trace
    };

    /// Standardised interface for logging used by the toolkit.
    class CQPTOOLKIT_EXPORT ILogger
    {
    public:
        /// Change the level out output from the logger
        /// @param[in] level Message which are as or more severe as this will be printed
        virtual void SetOutputLevel(LogLevel level) = 0;
        /// Gets the current setting of the filter for logging
        /// @return The currently limited log level
        virtual LogLevel GetOutputLevel() const = 0;
        /// Increase the level of output
        virtual void IncOutputLevel() = 0;
        /// Decrease the level of output
        virtual void DecOutputLevel() = 0;

        /// Send output to the logger
        /// @param[in] level The severity of the message
        /// @param[in] message The text to display
        virtual void Log(LogLevel level, const std::string& message) = 0;

        /// Daisy chain another logger so that it will receive the same messages as the top level logger.
        /// @param[in] newLogger The logger to add to the chain
        virtual void AttachLogger(ILogger* const newLogger) = 0;

        /// Daisy chain another logger so that it will receive the same messages as the top level logger.
        /// @param[in] logger The logger to remove from the chain
        virtual void DettachLogger(ILogger* const logger) = 0;

        /// pure virtual destructor
        virtual ~ILogger() = default;
    };

}
