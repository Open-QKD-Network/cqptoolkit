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
#if defined(__unix__)
    #include <string.h>
    #include <sys/types.h>
    #include <time.h>
    #include <sys/time.h>
#elif defined(WIN32)
    #include <Windows.h>
#endif

namespace cqp
{

    /// The logger which will be used to propagate all log messages to other loggers and to users
    static Logger defaultLogger;

    std::string Logger::GetLastErrorString()
    {
#if defined(__unix__)
        return std::string(strerror(errno));
#elif defined(WIN32)
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return std::string(); //No error message has been recorded

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
#endif
    }

    std::string Logger::GetTimeStamp()
    {
        char datebuf[256] = {0};
        const unsigned int datebufSize = 256;
        struct tm localTimeResult;
        struct timeval tv;
        int result = gettimeofday(&tv, NULL);

        const time_t timeInSeconds = (time_t) tv.tv_sec;
        strftime(datebuf,
            datebufSize,
            "%Y%m%d-%H%M%S",
            localtime_r(&timeInSeconds, &localTimeResult));
        char msbuf[5];
        /* Dividing (without remainder) by 1000 rounds the microseconds
           measure to the nearest millisecond. */
        result = snprintf(msbuf, 5, ".%3.3ld", long(tv.tv_usec / 1000));
        if(result < 0)
        {
           // snprint can error (negative return code) and the compiler now generates a warning
           // if we don't act on the return code
           memcpy(msbuf, "0000", 5);
        }
        int datebufCharsRemaining = datebufSize - (int)strlen(datebuf);
        strncat(datebuf, msbuf, datebufCharsRemaining - 1);
        datebuf[datebufSize - 1] = '\0';

        return std::string(datebuf);
    }

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
