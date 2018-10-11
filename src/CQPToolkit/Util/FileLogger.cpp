#include "FileLogger.h"
/*!
* @file
* @brief CQP Toolkit - File Logger
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 04 March 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Util/FileLogger.h"
#include <iostream>
#include <fstream>
#include <string>
#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Util/FileIO.h"

namespace cqp
{
    using namespace std;
    /// The single instance of the file logger
    FileLogger* theFileLogger = nullptr;

    FileLogger::FileLogger():
        Logger(),
        fout(&fileBuffer)
    {
        outputFilename = fs::GetHomeFolder() + fs::GetPathSep() + fs::GetApplicationName() + extension;

        SetFilename(outputFilename);
        // Attach ourselves as a logger
        DefaultLogger().AttachLogger(this);
    }

    FileLogger::~FileLogger()
    {
        DefaultLogger().DettachLogger(this);
    }

    void FileLogger::Log(LogLevel level, const std::string& message)
    {
        using namespace std;

        if (fileBuffer.is_open() && (level > LogLevel::Silent) && (level <= currentOutput))
        {
            lock_guard<mutex> lock(fileLock);
            fout << LEVELPREFIX.at(level) << message << endl << flush;
        }

        // Up call to parent class
        Logger::Log(level, message);

    }

    void FileLogger::SetFilename(const std::string& filename)
    {
        using namespace std;
        {
            lock_guard<mutex> lock(fileLock);
            fileBuffer.close();
            fileBuffer.open(filename, std::ios::out);
        }
        LOGINFO("Logfile opened: " + filename);
    }
    void FileLogger::Enable()
    {
        if (theFileLogger == nullptr)
        {
            theFileLogger = new FileLogger();
        }
    }
}
