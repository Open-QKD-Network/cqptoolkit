/*!
* @file
* @brief Stat
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPAlgorithms/Statistics/Stat.h"

namespace cqp
{
    namespace stats
    {
        const std::chrono::milliseconds ProcessingWorker::timeout {500};
        std::weak_ptr<ProcessingWorker> ProcessingWorker::me;

        StatBase::StatBase(const std::vector<std::string>& pathin, Units k) :
            path(pathin), units(k), uniqueId(Counter())
        {
            worker = ProcessingWorker::Instance();
        }

        double StatBase::GetRate() const
        {
            return rate;
        }

        Units StatBase::GetUnits() const
        {
            return units;
        }

        std::chrono::system_clock::time_point StatBase::GetUpdated() const
        {
            return updated;
        }

        size_t StatBase::GetId() const
        {
            return uniqueId;
        }

        const std::vector<std::string>&StatBase::GetPath() const
        {
            return path;
        }

        void StatBase::Reset()
        {
            using std::chrono::high_resolution_clock;
            updated = high_resolution_clock::now();
            modified = false;
            rate = {};
        }

        void StatBase::StopProcessingThread()
        {

        }

        size_t StatBase::Counter()
        {
            // the static variable maintains its state for the life of the program
            static std::atomic<size_t> count(0);
            return count++;
        }

        void ProcessingWorker::Worker()
        {
            using namespace std;
            ObjectList processList;
            while(!stopProcessing)
            {
                /*lock scope*/{

                    unique_lock<mutex> lock(processMutex);
                    bool dataWaiting = processCv.wait_for(lock, timeout, [&](){
                        return !waitingObjects.empty();
                    });

                    if(dataWaiting)
                    {
                        processList = waitingObjects;
                        waitingObjects.clear();
                    }
                }/*lock scope*/

                if(!stopProcessing)
                {
                    for(auto obj : processList) {
                        obj->ProcessStats();
                    }
                    processList.clear();
                }
            }
        }

        ProcessingWorker::ProcessingWorker() :
            processingThread(std::thread(&ProcessingWorker::Worker, this))
        {

        }

        std::shared_ptr<ProcessingWorker> ProcessingWorker::Instance()
        {
            static std::mutex m;
            std::lock_guard<std::mutex> lock(m);
            std::shared_ptr<ProcessingWorker> result = me.lock();
            if(!result)
            {
                result.reset(new ProcessingWorker());
                me = result;
            }
            return result;
        }

        void ProcessingWorker::Enque(StatBase* me)
        {
            using namespace std;
            lock_guard<mutex> lock(processMutex);
            waitingObjects.insert(me);
            processCv.notify_one();
        }

        ProcessingWorker::~ProcessingWorker()
        {
            stopProcessing = true;
            if(processingThread.joinable())
            {
                processingThread.join();
            }
        }

    } // namespace stats
} // namespace cqp


