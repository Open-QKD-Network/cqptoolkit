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

    /**
     * @brief The ProcessingQueue class uses multiple threads to process a list of data
     */
    class ALGORITHMS_EXPORT ProcessingQueue
    {
    public:
        /// The function to perform on the data
        using Action = std::function<void()>;

        /**
         * @brief ProcessingQueue constructor
         * @param numThreads The number of threads to process the queue with.
         * Default: The number of physical cores
         */
        ProcessingQueue(uint numThreads = std::thread::hardware_concurrency());

        /// Destructor
        ~ProcessingQueue();

        /**
         * @brief SetPriority
         * Change a threads priority
         * @param niceLevel Higher number == less chance it will run, more nice
         * @param realtimePriority Higher number == more chance it will run
         * @param policy The kind of scheduler to use
         * @return true on success
         */
        bool SetPriority(int niceLevel = 0, Scheduler policy = Scheduler::Normal, int realtimePriority = 1);

        /**
         * @brief Enqueue
         * Add a function to process to the queue
         * @details Copy version
         * @param action
         */
        void Enqueue(const Action& action);

        /**
         * @brief Enqueue
         * Add a funciton to process to the queue
         * @details Move version
         * @param action
         */
        void Enqueue(Action&& action);

    protected: // methods
        /**
         * @brief Processor Entry point for the processing threads
         */
        void Processor(); // Processor

    protected: // members
        /// Actions yet to be performed
        std::queue<Action> pending;

        ///protect access to the pending queue
        std::mutex pendingMutex;
        ///protect access to the pending queue
        std::condition_variable pendingCv;
        /// The processing threads
        std::vector<std::thread> threads;
        /// controls when the threads exit
        bool stopProcessing = false;
    };

} // namespace cqp
