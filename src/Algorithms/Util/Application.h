/*!
* @file
* @brief Application
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <vector>
#include <string>
#include "Algorithms/Util/CommandArgs.h"
#include "Algorithms/algorithms_export.h"
#include <csignal>
#include <condition_variable>
#include <memory>
#include <atomic>

namespace cqp
{
    /**
     * @brief The Application class
     * Provides a main entry application with built in command line handling
     */
    class ALGORITHMS_EXPORT Application
    {
    public:
        /**
         * @brief Application
         * Constructor
         */
        Application() noexcept;

        /// Destructor
        virtual ~Application() = default;

        /**
         * @brief Main
         * Override this to implement your application
         * Up call to it to process the command line arguments.
         * @param args command line arguments from main entry
         * @return application error code, 0 usually means success
         */
        virtual int Main(const std::vector<std::string> &args);

        /**
         * @brief main
         * Standard main entry routing, called by the CQP_MAIN macro
         * @param argc number of arguments
         * @param argv argument vector
         * @return Application exit code
         */
        int main(int argc, const char* argv[]);

        /**
         * @brief HandleVersion
         * Prints the version of the application and quits
         * @param option
         */
        void HandleVersion(const CommandArgs::Option& option);

        /// The function signiture for a signal
        using SignalFunction = std::function<void(int)>;

        /**
         * @brief AddSignalHandler registers a function to be called when the signal occours
         * @param signum The signal to react to
         * @param func The funciton to call
         * @return True on success
         */
        bool AddSignalHandler(int signum, SignalFunction func);

        /**
         * @brief WaitForShutdown blocks until something stops the application
         */
        virtual void WaitForShutdown();

        /**
         * @brief ShutdownNow stops the application
         */
        virtual void ShutdownNow();
    protected:
        /// command line switches
        CommandArgs definedArguments;
        /// Standard exit code for invalid arguments
        const int ErrorInvalidArgs = -1;
        /// The current value of the applications exit code
        int exitCode = 0;
        /// Indication that the Main method should return to allow the program to exit.
        bool stopExecution = false;
        /// The static instance used for signal handling
        /// it will be set by calling AddSignalHandler
        static std::unordered_map<int, SignalFunction> signalHandlers;
        /**
         * @brief SignalHandler
         * Callback from a signal which will map to a handler
         * @param signal The signal being thrown
         */
        static void SignalHandler(int signum);


    private:

        /// should the application stop
        std::atomic_bool shutdown {false};
        /// access control for shutdown
        std::mutex shutdownMutex;
        /// listen for changes to shutdown
        std::condition_variable waitForShutdown;
    };

    /// Macro for creating a standard main entry into a program
    /// @param name The class name to instantiate.
    /// This must have a function called `main` or be descended from Application
#if defined(__unix__)
#define CQP_MAIN(name) \
    int main(int argc, const char* argv[]) \
    { \
        __try{ \
            name instance; \
            return instance.main(argc, argv); \
        } __catch(const std::exception& e) \
        { \
            LOGERROR(e.what()); \
            return -1; \
        } \
    }
#elif defined(WIN32)

#define CQP_MAIN(name) \
    int main(int argc, const char* argv[]) \
    { \
        __try{ \
            name instance; \
            return instance.main(argc, argv); \
        } __finally() \
        { \
            LOGERROR(e.what()); \
            return -1; \
        } \
    }
#endif
} // namespace cqp


