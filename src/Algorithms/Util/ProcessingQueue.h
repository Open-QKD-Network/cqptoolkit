#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Util/Threading.h"

namespace cqp
{

    class ALGORITHMS_EXPORT ProcessingQueue
    {
    public:
        using Action = std::function<void()>;

        ProcessingQueue(uint numThreads = std::thread::hardware_concurrency())
        {
            threads.reserve(numThreads);
            for(auto index = 0u; index < numThreads; index++)
            {
                threads.emplace_back(std::thread(&ProcessingQueue::Processor, this));
            }
        }

        ~ProcessingQueue()
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

        bool SetPriority(int nice = 0, Scheduler policy = Scheduler::Normal, int realtimePriority = 1)
        {
            bool result = true;
            for(auto index = 0u; index < threads.size(); index++)
            {
                result &= cqp::SetPriority(threads[index], nice, policy, realtimePriority);
            }

            return result;
        }

        /**
         * @brief Enqueue
         * Add a function to process to the queue
         * @details Copy version
         * @param action
         */
        void Enqueue(const Action& action)
        {
            using namespace std;

            { /* lock scope*/
                lock_guard<mutex> lock(pendingMutex);
                pending.push(action);
                totalEnqueued++;
            }/*lock scope*/

            pendingCv.notify_one();
        }

        /**
         * @brief Enqueue
         * Add a funciton to process to the queue
         * @details Move version
         * @param action
         */
        void Enqueue(Action&& action)
        {
            using namespace std;

            { /* lock scope*/
                lock_guard<mutex> lock(pendingMutex);
                pending.push(action);
                totalEnqueued++;
            }/*lock scope*/

            pendingCv.notify_one();
        }

        size_t GetTotalEnqueued() const
        {
            return totalEnqueued;
        }

        void ResetTotalEnqueued()
        {
            totalEnqueued = 0;
        }

    protected: // methods
        void Processor()
        {
            using namespace std;

            Action action;

            while(!stopProcessing)
            {
                { /* lock scope*/
                    unique_lock<mutex> lock(pendingMutex);

                    pendingCv.wait(lock, [this](){
                        return !pending.empty() || stopProcessing;
                    });

                    if(!stopProcessing)
                    {
                        action = move(pending.front());
                        pending.pop();
                    }
                }/*lock scope*/

                if(!stopProcessing)
                {
                    action();
                }
            } // while keep processing
        } // Processor

    protected: // members
        size_t totalEnqueued = 0;
        std::queue<Action> pending;

        std::mutex pendingMutex;
        std::condition_variable pendingCv;
        std::vector<std::thread> threads;

        bool stopProcessing = false;
    };

} // namespace cqp
