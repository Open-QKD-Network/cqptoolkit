#pragma once
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Util/Threading.h"
#include <functional>
#include <queue>
#include <future>

namespace cqp
{
    /**
     * @brief The ProcessingQueue class uses multiple threads to process a list of data
     */
    template<typename Return = void>
    class ALGORITHMS_EXPORT ProcessingQueue : public threads::ThreadManager
    {
    public:
        /// The function to perform on the data
        using Action = std::function<Return ()>;

        /**
         * @brief ProcessingQueue constructor
         * @param numThreads The number of threads to process the queue with.
         * Default: The number of physical cores
         */
        ProcessingQueue(uint numThreads = std::thread::hardware_concurrency());

        /**
         * @brief Enqueue
         * Add a funciton to process to the queue.
         * Can be called with a lambda:
         * @code
         * ProcessingQueue<int> worker;
         * auto result = worker.Enqueue([&]() {
         *      return 1+2;
         * });
         * std::cout << "1 + 2 = " << result.get() << std::end;
         * @endcode
         * @note get() in the future can only be called once.
         * @details Copy version
         * @param action The action to be performed. The function must take no arguments and return the type specified in the 'Return' template parameter
         * @return A std::future object which will contain the result of the function once it's been called.
         */
        std::future<Return> Enqueue(const Action& action);

        /**
         * @copydoc Enqueue
         * @details Move version
         */
        std::future<Return> Enqueue(Action&& action);

    protected: // methods
        /**
         * @brief Processor Entry point for the processing threads
         */
        void Processor() override;

    protected: // members
        /// The promise type for the given return type
        using Promise = std::promise<Return>;

        /// Storage tpye for the combination of the function and it's return promise
        using TaskPair = std::pair<Action, Promise>;
        /// Actions yet to be performed
        std::queue<TaskPair> pending;

    };

    template<typename Return>
    ProcessingQueue<Return>::ProcessingQueue(uint numThreads)
    {
        ConstructThreads(numThreads);
    }

    template<typename Return>
    std::future<Return> ProcessingQueue<Return>::Enqueue(const Action& action)
    {
        using namespace std;
        std::future<Return> result;

        {
            /* lock scope*/
            lock_guard<mutex> lock(pendingMutex);
            auto prom = std::promise<Return>();
            result = prom.get_future();
            pending.emplace(move(action), move(prom));
        }/*lock scope*/

        pendingCv.notify_one();
        return std::move(result);
    }

    template<typename Return>
    std::future<Return> ProcessingQueue<Return>::Enqueue(Action&& action)
    {
        using namespace std;
        std::future<Return> result;

        {
            /* lock scope*/
            lock_guard<mutex> lock(pendingMutex);
            auto prom = std::promise<Return>();
            result = prom.get_future();
            pending.emplace(move(action), move(prom));
        }/*lock scope*/

        pendingCv.notify_one();
        return result;
    }

    template<typename Return>
    void cqp::ProcessingQueue<Return>::Processor()
    {
        using namespace std;

        TaskPair item;

        while(!stopProcessing)
        {
            {
                /* lock scope*/
                unique_lock<mutex> lock(pendingMutex);

                pendingCv.wait(lock, [this]()
                {
                    return !pending.empty() || stopProcessing;
                });

                if(!stopProcessing)
                {
                    item = move(pending.front());

                    pending.pop();
                }
            }/*lock scope*/

            if(!stopProcessing)
            {
                item.second.set_value(item.first());
            }
        } // while keep processing
    }
} // namespace cqp
