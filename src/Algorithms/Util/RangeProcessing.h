#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Util/Threading.h"


namespace cqp {

    template<typename RangeType>
    class ALGORITHMS_EXPORT RangeProcessing
    {
    public:
        using Action = std::function<void(RangeType)>;

        RangeProcessing(uint numThreads = std::thread::hardware_concurrency())
        {
            threads.reserve(numThreads);
            for(auto index = 0u; index < numThreads; index++)
            {
                threads.emplace_back(std::thread(&RangeProcessing::Processor, this));
            }
        }

        ~RangeProcessing()
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

        /**
         * The funtion controls which value is processed next and whether to stop
         * The funciton returns true if there are more values.
         */
        using NextValueFunc = std::function<bool(RangeType&)>;

        void ProcessSequence(Action process, NextValueFunc nextVal)
        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            action = process;

            nextValueFunc = nextVal;
            moreValuesAvailable = true;
            pendingCv.notify_all();
        }

        size_t GetTotalIterations() const
        {
            return totalIterations;
        }

    protected: // methods

        void Processor()
        {
            using namespace std;

            while(!stopProcessing)
            {
                RangeType nextVal;
                bool processValue = false;

                { /* lock scope*/
                    unique_lock<mutex> lock(pendingMutex);

                    pendingCv.wait(lock, [&](){
                        bool unblock = false;

                        if(stopProcessing)
                        {
                            unblock = true;
                        } else if(moreValuesAvailable)
                        {
                            moreValuesAvailable = nextValueFunc(nextVal);
                            processValue = true;
                            totalIterations++;
                            unblock = true;
                        }

                        return unblock;
                    });
                } /*lock scope*/

                if(processValue)
                {
                    action(nextVal);
                }

            } // while keep processing
        } // Processor

    protected: // members
        size_t totalIterations = 0;
        NextValueFunc nextValueFunc;
        Action action;
        std::vector<std::thread> threads;
        std::mutex pendingMutex;
        std::condition_variable pendingCv;
        bool moreValuesAvailable = false;
        bool stopProcessing = false;
    };

} // namespace cqp


