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
#include "WorkerThread.h"

#include <thread>
#include <condition_variable>
#include <mutex>
#include "CQPToolkit/Util/Logger.h"
#include "Platform.h"

#if defined(__linux)
    #include <sys/time.h>
    #include <sys/resource.h>
    #include <cstring>
    #include <sched.h>
    #include <unistd.h>
#elif defined(WIN32)
    #include <Processthreadsapi.h>
#endif

namespace cqp
{
    using namespace std;

    void WorkerThread::ThreadExec()
    {
        LOGTRACE("WorkerThread::ThreadExec Woke up");

        while(state == State::Started)
        {
            try
            {
                DoWork();
            }
            catch (const exception& e)
            {
                LOGERROR("WorkerThread threw an exception: " + std::string(e.what()));
            }
        }
        LOGTRACE("WorkerThread::ThreadExec Stopping");
    }

    WorkerThread::WorkerThread()
    {

    }

    WorkerThread::~WorkerThread()
    {
        WorkerThread::Stop(true);
    }

    void WorkerThread::Start(int nice, Scheduler policy, int realtimePriority)
    {
        lock_guard<mutex> lock(accessMutex);
        // Start the task if need be.
        if(state == State::NotStarted)
        {
            LOGTRACE("Thread Starting.");
            state = State::Started;
            worker = thread(&WorkerThread::ThreadExec, this);
            if(nice != 0 || policy != Scheduler::Normal)
            {
                SetPriority(nice, policy, realtimePriority);
            }
        }
        // if it's already running, don't do anything.
    }

    void WorkerThread::Stop(bool wait)
    {
        {
            LOGTRACE("Thread Stopping...");
            lock_guard<mutex> lock(accessMutex);
            state = State::Stop;
        }

        if (wait )
        {
            Join();
        }
        else if(worker.joinable())
        {
            worker.detach();
        }
        LOGTRACE("Thread Stopped.");
    }

    void WorkerThread::Join()
    {
        if (worker.joinable())
        {
            LOGTRACE("Waiting for thread");
            worker.join();
        }
    }

    bool WorkerThread::IsRunning()
    {
        bool result = false;
        {
            lock_guard<mutex> lock(accessMutex);
            result = state == State::Started;
        }
        return result;
    }

    bool WorkerThread::SetPriority(thread& theThread, int niceLevel, WorkerThread::Scheduler policy, int priority)
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
                LOGWARN("Priority for real time schedular must be > 0, setting to 1, see man sched 7");
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
        rsult &= SetThreadPriority(theThread.native_handle(), priority) != 0;

        if(!result)
        {
            LOGERROR("Failed to set thread priority");
        }
#endif

        return result;
    }

    bool WorkerThread::SetPriority(int niceLevel, WorkerThread::Scheduler policy, int realtimePriority)
    {
        return SetPriority(worker, niceLevel, policy, realtimePriority);
    }

    bool WorkerThread::ShouldStop()
    {
        lock_guard<mutex> lock(accessMutex);
        return (state == State::Stop);
    }
}
