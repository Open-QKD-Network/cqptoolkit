/*!
* @file
* @brief CQP Toolkit - Worker thread helper
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Util/Threading.h"

namespace cqp
{
    /// Standard thread control utility class
    /// @details
    /// Example usage:
    /// @code
    /// MyEventWorker : public WorkerThread
    /// {
    ///     void DoWork();
    /// }
    /// MyEventWorker eventWorker;
    ///
    /// void main() {
    ///     eventWorker.Start();
    /// }
    /// @endcode
    class ALGORITHMS_EXPORT WorkerThread
    {
    public:
        /// Default constructor for a worker
        WorkerThread() = default;
        /// Default destructor
        /// @details This will wait for the thread to complete
        virtual ~WorkerThread();

        /// Allow work to be done by the WorkerThread::DoWork() method
        /// @details This has no effect if the thread is already started.
        /// If the thread has been previously stopped, it will be restarted.
        /// @param nice Higher number == less chance it will run, more nice
        /// @param realtimePriority Higher number == more chance it will run
        /// @param policy The kind of scheduler to use
        virtual void Start(int nice = 0, threads::Scheduler policy = threads::Scheduler::Normal, int realtimePriority = 1);
        /// Signal the worker thread to stop what it's doing.
        /// @details The WorkerThread::DoWork() call must provide a means of being interrupted if this is not going to block.
        /// @param[in] wait If the thread is running and this is true, the call will not return until the worker thread completes.
        virtual void Stop(bool wait = false);

        /// Check if the work thread is currently running
        /// @return true if the state is Started
        virtual bool IsRunning();

        /**
         * @brief SetPriority
         * Change a threads priority
         * @param niceLevel Higher number == less chance it will run, more nice
         * @param realtimePriority Higher number == more chance it will run
         * @param policy The kind of scheduler to use
         * @return true on success
         */
        bool SetPriority(int niceLevel, threads::Scheduler policy = threads::Scheduler::Normal, int realtimePriority = 1);

    protected:
        /// Member function for performing work on the separate thread.
        /// @details The WorkerThread will call this when the thread is allowed to run,
        /// if the function returns it will be called repeatedly until WorkerThread::Stop() is called.
        /// If this thread doesn't return, the parent will wait indefinitely for it when WorkerThread::Stop() is called.
        /// @remarks This function is wrapped in an exception handler to ensure that the
        /// thread is never killed unless it is explicitly stopped
        virtual void DoWork() = 0;

        /// The commanded state of the thread
        enum class State
        {
            NotStarted, Started, Stop
        };
        /// The commanded state of the thread
        State state = State::NotStarted;
        /// Utility function for implementers to check if their WorkerThread::DoWork() function should return to allow the thread to be stopped.
        /// @returns true if the thread hsa been told to stop
        bool ShouldStop();

        /// @internal

        /// Controls access to various variables used to control execution
        std::mutex accessMutex;
        /// The thread on which the WorkerThread::ThreadExec() runs
        std::thread worker;
        /// A conditional to control the thread execution
        /// this will be triggered by events such as new data and commands to stop the thread
        std::condition_variable threadConditional;
        /// Method for managing execution of the thread, this will call WorkerThread::DoWork() as necessary
        void ThreadExec();
    };

}
