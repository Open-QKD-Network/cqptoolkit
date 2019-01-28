#include "ProcessingQueue.h"

cqp::ProcessingQueue::ProcessingQueue(uint numThreads)
{
    threads.reserve(numThreads);
    for(auto index = 0u; index < numThreads; index++)
    {
        threads.emplace_back(std::thread(&ProcessingQueue::Processor, this));
    }
}

cqp::ProcessingQueue::~ProcessingQueue()
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

bool cqp::ProcessingQueue::SetPriority(int niceLevel, cqp::Scheduler policy, int realtimePriority)
{
    bool result = true;
    for(auto index = 0u; index < threads.size(); index++)
    {
        result &= cqp::SetPriority(threads[index], niceLevel, policy, realtimePriority);
    }

    return result;
}

void cqp::ProcessingQueue::Enqueue(const cqp::ProcessingQueue::Action& action)
{
    using namespace std;

    { /* lock scope*/
        lock_guard<mutex> lock(pendingMutex);
        pending.push(action);
    }/*lock scope*/

    pendingCv.notify_one();
}

void cqp::ProcessingQueue::Enqueue(cqp::ProcessingQueue::Action&& action)
{
    using namespace std;

    { /* lock scope*/
        lock_guard<mutex> lock(pendingMutex);
        pending.push(action);
    }/*lock scope*/

    pendingCv.notify_one();
}

void cqp::ProcessingQueue::Processor()
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
}
