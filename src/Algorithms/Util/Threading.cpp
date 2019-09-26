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
#include "Algorithms/Util/Threading.h"
#include "Algorithms/Logging/Logger.h"

#if defined(__linux)
    #include <sys/time.h>
    #include <sys/resource.h>
    #include <cstring>
    #include <sched.h>
    #include <unistd.h>
#elif defined(WIN32)
#include <windows.h>
#include <tchar.h>
#endif

namespace cqp {

    namespace threads {
        bool SetPriority(std::thread& theThread, int niceLevel, Scheduler policy, int priority)
        {
            bool result = false;
    #if defined(__linux)
            int schedVal = 0;
            sched_param schParams {};

            if(policy == Scheduler::RoundRobin || policy == Scheduler::FIFO || policy == Scheduler::Deadline)
            {
                // real time
                if(priority <= 0)
                {
                    LOGWARN("Priority for real time scheduler must be > 0, setting to 1, see man sched 7");
                    schParams.sched_priority = 1;
                }
                else
                {
                    schParams.sched_priority = priority;
                }
            }
            else
            {
                schParams.sched_priority = 0;
            }

            switch (policy)
            {
            case Scheduler::Idle:
                schedVal = SCHED_IDLE;
                break;
            case Scheduler::Batch:
                schedVal = SCHED_BATCH;
                break;
            case Scheduler::Normal:
                schedVal = SCHED_OTHER;
                break;
            case Scheduler::RoundRobin:
                schedVal = SCHED_RR;
                break;
            case Scheduler::FIFO:
                schedVal = SCHED_FIFO;
                break;
            case Scheduler::Deadline:
                schedVal = SCHED_DEADLINE;
                break;
            }

            result = pthread_setschedparam(theThread.native_handle(), schedVal, &schParams) == 0;
            if(!result)
            {
                LOGERROR("Failed to set Thread scheduling : " + std::strerror(errno));
            }

            // reset this now so we can check it later
            errno = 0;
            // nice returns the new nice level or -1 for error :/
            if(nice(niceLevel) == -1 && errno != 0)
            {
                result = false;
                LOGERROR("Failed to set nice level to " + std::to_string(niceLevel));
            }

    #elif defined(WIN32)
            DWORD threadClass = 0;
            switch (policy)
            {
            case Scheduler::Idle:
                threadClass = IDLE_PRIORITY_CLASS;
                break;
            case Scheduler::Batch:
                threadClass = BELOW_NORMAL_PRIORITY_CLASS;
                break;
            case Scheduler::Normal:
                threadClass = NORMAL_PRIORITY_CLASS;
                break;
            case Scheduler::RoundRobin:
                threadClass = ABOVE_NORMAL_PRIORITY_CLASS;
                break;
            case Scheduler::FIFO:
                threadClass = HIGH_PRIORITY_CLASS;
                break;
            case Scheduler::Deadline:
                threadClass = REALTIME_PRIORITY_CLASS;
                break;
            }

            result = SetPriorityClass(theThread.native_handle(), threadClass) != 0;
            if(!result)
            {
                LOGERROR("Failed to set priority class");
            }
            result &= SetThreadPriority(theThread.native_handle(), priority) != 0;

            if(!result)
            {
                LOGERROR("Failed to set thread priority");
            }
    #endif

            return result;
        }

        ThreadManager::~ThreadManager()
        {
            stopProcessing = true;
            pendingCv.notify_all();

            for(auto& worker : threads)
            {
                if(worker.joinable())
                {
                    worker.join();
                }
            }
        }

        bool ThreadManager::SetPriority(int niceLevel, Scheduler policy, int realtimePriority)
        {
            bool result = true;
            for(auto& thread : threads)
            {
                result &= threads::SetPriority(thread, niceLevel, policy, realtimePriority);
            }

            return result;
        }

        void ThreadManager::ConstructThreads(uint32_t numThreads)
        {
            threads.reserve(numThreads);
            for(auto index = 0u; index < numThreads; index++)
            {
                threads.emplace_back(std::thread(&ThreadManager::Processor, this));
            }
        }
    } // namespace threads
} // namespace cqp
