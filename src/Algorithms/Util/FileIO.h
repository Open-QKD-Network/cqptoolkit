/*!
* @file
* @brief CQP Toolkit - File utility functions
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/algorithms_export.h"
#include <vector>
#include <limits>
#include <string>

#if defined(_WIN32)
    /// The definition of a file handle on the current platform
    #define FILE_HANDLE HANDLE
#else //defined(_WIN32)
    /// @def FILE_HANDLE
    /// The definition of a file handle on the current platform
    #define FILE_HANDLE int
#endif // defined(_WIN32)

namespace cqp
{
    namespace fs
    {
        /// Get the directory owned by the user
        /// @details This should be writable by the user. On windows this equates
        /// to the users My Documents folder
        /// On Linux this is the value of $HOME
        /// @return A string with the full path the users home path
        ALGORITHMS_EXPORT std::string GetHomeFolder();

        /// Gets the correct separator for paths in the current OS
        /// @return a character which can be used to separate folder names
        ALGORITHMS_EXPORT std::string GetPathSep();

        /// Gets the correct separator for paths defined in the PATH environment variable for the current OS
        /// @return a character which is used to separate paths in the PATH variable
        ALGORITHMS_EXPORT std::string GetPathEnvSep();

        /// open a browser window at the URL specified using the users default browser
        /// @param[in] url The URL to display
        /// @return true on success
        ALGORITHMS_EXPORT bool OpenURL(const std::string& url);

        /**
         * @brief Exists
         * @param filename
         * @return true of the file exists
         */
        ALGORITHMS_EXPORT bool Exists(const std::string& filename);

        /**
         * @brief ReadEntireFile
         * Read the contents of a file into the string output
         * @param[in] filename
         * @param[out] output
         * @param[in] limit Read a maximum of limit bytes
         * @return true on success
         */
        ALGORITHMS_EXPORT bool ReadEntireFile(const std::string& filename, std::string& output, size_t limit = std::numeric_limits<size_t>::max());
        /**
         * @brief WriteEntireFile
         * Overwrite the file with the contents
         * @param filename
         * @param contents
         * @return true on success
         */
        ALGORITHMS_EXPORT bool WriteEntireFile(const std::string& filename, const std::string& contents);

        /**
         * @brief Parent
         * Get the parent path of the full path to a filename
         * @param path full path to split
         * @return The path up until the final path separator
         */
        ALGORITHMS_EXPORT std::string Parent(const std::string& path);
        /**
         * @brief BaseName
         * Get the filename from a full path
         * @param path
         * @return the filename after the final path separator
         */
        ALGORITHMS_EXPORT std::string BaseName(const std::string& path);

        /**
         * @brief CreateDirectory
         * @param path Directory to create
         * @return  true on success
         */
        ALGORITHMS_EXPORT bool CreateDirectory(const std::string& path);
        /**
         * @brief IsDirectory
         * @param path
         * @return true if the path is a directory
         */
        ALGORITHMS_EXPORT bool IsDirectory(const std::string& path);
        /**
         * @brief CanWrite
         * @param path
         * @return true if the path is accessible by the user
         */
        ALGORITHMS_EXPORT bool CanWrite(const std::string& path);
        /**
         * @brief MakeTemp
         * Create a uniquely named temporary file/directory
         * @param directory if true, create a directory, otherwise create a file
         * @return the path of the file/directory
         */
        ALGORITHMS_EXPORT std::string MakeTemp(bool directory = false);
        /**
         * @brief Delete
         * Delete a file or directory
         * @param path
         * @return true on success
         */
        ALGORITHMS_EXPORT bool Delete(const std::string& path);

        /**
         * @brief GetCurrentPath
         * @return the current working directory
         */
        ALGORITHMS_EXPORT std::string GetCurrentPath();
        /**
         * @brief ListChildren
         * @param path
         * @return names of immediate children
         */
        ALGORITHMS_EXPORT std::vector<std::string> ListChildren(const std::string& path);
        /**
         * @brief FindGlob
         * @param search search criteria
         * @return items found
         */
        ALGORITHMS_EXPORT std::vector<std::string> FindGlob(const std::string& search);

        /**
         * @brief FullPath
         * @param relPath partial path
         * @return the full path for the supplied filename
         */
        ALGORITHMS_EXPORT std::string FullPath(const std::string& relPath);
    } // namespace fs
} // namespace cqp
