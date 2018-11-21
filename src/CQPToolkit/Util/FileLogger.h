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
#pragma once
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Util/Platform.h"
#include <mutex>
#include <fstream>

namespace cqp
{
    /// @brief Log output to a file
    /// @details The file defaults to
    /// the name of the program being run in the users home folder.
    class CQPTOOLKIT_EXPORT FileLogger :
        public Logger
    {
    public:

        /// Standard destructor
        virtual ~FileLogger();

        /// @brief Output a message at a given deverity level
        /// @note The current output level may hide the message.
        /// @param[in] level The severity of the log message
        /// @param[in] message The message to display
        virtual void Log(LogLevel level, const std::string& message) override;

        /// Change the destination of the output.
        /// Only subsequent messages will be sent to the file.
        /// @param[in] filename The filename to log to
        void SetFilename(const std::string& filename);
        /// Start using the logger
        /// Calling this when already enabled will have no effect
        static void Enable();
    protected:
        /// Default constructor
        FileLogger();
        /// The stream to which messages are sent
        std::ostream fout;
        /// Links the stream to a file
        std::filebuf fileBuffer;
        /// The name of the file being written to.
        std::string outputFilename;
        /// The extension added to the output filename
        const std::string extension = ".txt";
    private:
        /// Prevent multiple threads from spamming the file
        std::mutex fileLock;
    };

}
