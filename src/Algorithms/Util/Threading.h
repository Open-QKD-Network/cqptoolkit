/*!
* @file
* @brief CQP Toolkit - Worker thread helper
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28 Jan 2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "Algorithms/algorithms_export.h"

namespace cqp
{
    namespace threads {
        /// platform independent definitions of scheduling methods
        /// these do not directly translate on every platform
        enum class Scheduler { Idle, Batch, Normal, RoundRobin, FIFO, Deadline};

        /**
         * @brief SetPriority
         * Change a threads priority
         * @param theThread
         * @param niceLevel Higher number == less chance it will run, more nice
         * @param realtimePriority Higher number == more chance it will run
         * @param policy The kind of scheduler to use
         * @return true on success
         */
        ALGORITHMS_EXPORT bool SetPriority(std::thread& theThread, int niceLevel, Scheduler policy = Scheduler::Normal, int realtimePriority = 1);

        /**
         * @brief The ThreadManager class provides generatic threading functions
         */
        class ALGORITHMS_EXPORT ThreadManager
        {
        public:

            /// Distructor
            virtual ~ThreadManager();

            /**
             * @brief SetPriority
             * Change a threads priority
             * @param niceLevel Higher number == less chance it will run, more nice
             * @param realtimePriority Higher number == more chance it will run
             * @param policy The kind of scheduler to use
             * @return true on success
             */
            bool SetPriority(int niceLevel = 0, Scheduler policy = Scheduler::Normal, int realtimePriority = 1);

        protected: // methods
            /**
             * @brief Processor Entry point for the processing threads
             */
            virtual void Processor() = 0; // Processor

            /**
             * @brief ConstructThreads
             * Create the threads, called by the calss which implements the Processor function on creation.
             * @param numThreads The numnber of threads to create
             */
            void ConstructThreads(uint32_t numThreads = std::thread::hardware_concurrency());

        protected: // members
            /// The processing threads
            std::vector<std::thread> threads;
            ///protect access to the pending queue
            std::mutex pendingMutex;
            ///protect access to the pending queue
            std::condition_variable pendingCv;
            /// controls when the threads exit
            bool stopProcessing = false;
        };
    } // namespace threads
} // namespace cqp
