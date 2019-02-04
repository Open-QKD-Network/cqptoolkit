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
#include "Algorithms/Logging/Logger.h"

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

        threadConditional.notify_all();

        if (wait )
        {
            WorkerThread::Join();
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

    bool WorkerThread::SetPriority(int niceLevel, Scheduler policy, int realtimePriority)
    {
        return cqp::SetPriority(worker, niceLevel, policy, realtimePriority);
    }

    bool WorkerThread::ShouldStop()
    {
        lock_guard<mutex> lock(accessMutex);
        return (state == State::Stop);
    }
}
