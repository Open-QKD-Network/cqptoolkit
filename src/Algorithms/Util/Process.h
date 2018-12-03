/*!
* @file
* @brief Process
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include "Algorithms/algorithms_export.h"

namespace cqp
{

    /**
     * @brief The Process class
     * Manage external commands
     */
    class ALGORITHMS_EXPORT Process
    {
    public:

        /**
         * @brief Process
         */
        Process() = default;

        /// Destructor
        ~Process();

        /**
         * @brief Start
         * Launch a process on the system
         * @param command The command to execute
         * @param args command line arguments to pass to the command
         * @param[in] stdIn source for data to sent to commands std in
         * @param[in] stdOut sink for capturing output from the command
         * @param[in] stdErr sink for capturing output from the command
         * @return true on success
         */
        bool Start(const std::string& command, const std::vector<std::string>& args = {},
                   int* stdIn = nullptr,
                   int* stdOut = nullptr,
                   int* stdErr = nullptr);

        /**
         * @brief Running
         * @return true if the process is running
         */
        bool Running() const;
        /**
         * @brief RequestTermination
         * Attemp to terminate the process
         * @param wait if true, the call will block until the process is terminated
         */
        void RequestTermination(bool wait = false);

        /**
         * @brief WaitForExit
         * Block until the process exits
         * @return The return code of the process
         */
        int WaitForExit();
    protected:
        /// system id for the process
        int pid = 0;
    private:
        /**
         * @brief Fork
         * clone the process and handle the pipes
         * @param command
         * @param args
         * @param stdIn
         * @param stdOut
         * @param stdErr
         * @return true on success
         */
        bool Fork(const std::string& command, const std::vector<std::string>& args, int* stdIn, int* stdOut, int* stdErr);
    };

} // namespace cqp
