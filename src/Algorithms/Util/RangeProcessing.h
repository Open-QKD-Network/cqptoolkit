#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Util/Threading.h"


namespace cqp {

    /**
     * @brief RangeProcessing class allows a sequence of values to be applied to a processing function using multiple threads
     * @tparam RangeType The type passed to the iteration
     */
    template<typename RangeType>
    class ALGORITHMS_EXPORT RangeProcessing
    {
    public:
        /// The function to perform on each iteration
        /// The parameter is the value for the iteration
        using Action = std::function<void(RangeType)>;

        /**
         * @brief RangeProcessing constructor
         * @param numThreads The number of threads to process the queue with.
         * Default: The number of physical cores
         */
        RangeProcessing(uint numThreads = std::thread::hardware_concurrency());

        /// destructor
        ~RangeProcessing();

        /**
         * The funtion controls which value is processed next and whether to stop
         * The funciton returns true if there are more values.
         */
        using NextValueFunc = std::function<bool(RangeType&)>;

        /**
         * @brief ProcessSequence
         * Define a process to perform over a range of data values
         * @details
         * Example usage:
         * @code
         * RangeProcessing<size_t> processor;
         * // action to perform on every iteration, compare the values with an offset
         *
         *  std::mutex resultsMutex;
         *  std::condition_variable resultsCv;
         *  // counter to provide the next value in the sequence
         *  std::atomic_long counter(from);
         *  auto processLambda = [&](ssize_t offset){
         *      CompareValues(truth, irregular, offset);
         *
         *      {
         *          std::lock_guard<std::mutex> lock(resultsMutex);
         *          numProcessed++;
         *      }
         *
         *      resultsCv.notify_one();
         *  };
         *
         *  // function to return the next number in the sequence
         *  auto nextValLambda = [&](ssize_t& nextVal){
         *      nextVal = counter.fetch_add(1);
         *      return nextVal < to;
         *  };
         *
         *  // start the process
         *  rangeWorker.ProcessSequence(processLambda, nextValLambda);
         *
         *  // wait for all the values to be processed
         *  std::unique_lock<std::mutex> lock(resultsMutex);
         *  resultsCv.wait(lock, [&](){
         *      return numProcessed == totalIterations;
         *  });
         *
         * @endcode
         * @param process The function to perform, it will be passed a value provided by the nextVal function
         * @param nextVal A function which will return the next value in the sequence, once the function returns false, the processing will stop
         */
        void ProcessSequence(Action process, NextValueFunc nextVal);

        /**
         * @brief SetPriority
         * Change a threads priority
         * @param niceLevel Higher number == less chance it will run, more nice
         * @param realtimePriority Higher number == more chance it will run
         * @param policy The kind of scheduler to use
         * @return true on success
         */
        bool SetPriority(int niceLevel, Scheduler policy = Scheduler::Normal, int realtimePriority = 1);

    protected: // methods

        /**
         * @brief Processor Entry point for the processing threads
         * @details
         * @startuml
         * [-> RangeProcessing : Processor
         * activate RangeProcessing
         *
         *     loop until stopped
         *         RangeProcessing -> Caller : nextVal()
         *         RangeProcessing -> Caller : action(nextValue)
         *     end loop
         *
         * deactivate RangeProcessing
         * @enduml
         */
        void Processor();

    protected: // members
        /// funciton which provides the next value
        NextValueFunc nextValueFunc;
        /// function to process the next value
        Action action;
        /// The processing threads
        std::vector<std::thread> threads;
        ///protect access to the pending queue
        std::mutex pendingMutex;
        ///protect access to the pending queue
        std::condition_variable pendingCv;
        /// whether the sequence has been completed
        bool moreValuesAvailable = false;
        /// controls when the threads exit
        bool stopProcessing = false;
    }; //RangeProcessing class

    // RangeProcessing Implementation

    template<typename RangeType>
    RangeProcessing<RangeType>::RangeProcessing(uint numThreads)
    {
        threads.reserve(numThreads);
        for(auto index = 0u; index < numThreads; index++)
        {
            threads.emplace_back(std::thread(&RangeProcessing::Processor, this));
        }
    }

    template<typename RangeType>
    RangeProcessing<RangeType>::~RangeProcessing()
    {
        stopProcessing = true;
        // wake up all the threads so they can check the flag
        pendingCv.notify_all();
        // wait for the threads to exit
        for(auto& worker : threads)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }
    }

    template<typename RangeType>
    void RangeProcessing<RangeType>::ProcessSequence(RangeProcessing::Action process, RangeProcessing::NextValueFunc nextVal)
    {
        /*lock scope*/{
            std::lock_guard<std::mutex> lock(pendingMutex);
            action = process;

            nextValueFunc = nextVal;
            moreValuesAvailable = true;
        }/*lock scope*/

        // start the threads
        pendingCv.notify_all();
    }

    template<typename RangeType>
    bool RangeProcessing<RangeType>::SetPriority(int niceLevel, Scheduler policy, int realtimePriority)
    {
        bool result = true;
        for(auto index = 0u; index < threads.size(); index++)
        {
            result &= cqp::SetPriority(threads[index], niceLevel, policy, realtimePriority);
        }

        return result;
    }

    template<typename RangeType>
    void RangeProcessing<RangeType>::Processor()
    {
        using namespace std;

        while(!stopProcessing)
        {
            RangeType nextVal;
            bool processValue = false;

            { /* lock scope*/
                unique_lock<mutex> lock(pendingMutex);

                // wait for data or the command to quit
                pendingCv.wait(lock, [&](){
                    bool unblock = false;

                    if(stopProcessing)
                    {
                        unblock = true;
                    } else if(moreValuesAvailable)
                    {
                        moreValuesAvailable = nextValueFunc(nextVal);
                        processValue = true;
                        unblock = true;
                    }

                    return unblock;
                });
            } /*lock scope*/

            if(processValue)
            {
                // perform the work
                action(nextVal);
            }

        } // while keep processing
    }
} // namespace cqp


