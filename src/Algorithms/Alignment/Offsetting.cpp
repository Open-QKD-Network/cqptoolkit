#include "Offsetting.h"
#include <atomic>
#include <algorithm>

namespace cqp {
    namespace align {

        Offsetting::Offsetting(size_t samples):
            samples(samples),
            rangeWorker()
        {

        }

        Offsetting::Confidence Offsetting::HighestValue(const QubitsBySlot markers, const std::vector<SlotID>& validSlots, const QubitList& irregular, ssize_t from, ssize_t to)
        {
            std::mutex resultsMutex;
            std::condition_variable resultsCv;
            // counter to provide the next value in the sequence
            std::atomic_long counter(from);

            // result
            Confidence highest {0.0, 0};
            // the number of times the process will run
            const size_t totalIterations = to - from + 1;
            // the number of times the process has run
            size_t numProcessed = 0;

            // action to perform on every iteration, compare the values with an offset
            auto processLambda = [&](ssize_t offset){
                double confidence = CompareValues(markers, validSlots, irregular, offset);

                /*lock scope*/ {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    numProcessed++;
                    // store the value if it's the highest
                    if(confidence > highest.value)
                    {
                        highest.value = confidence;
                        highest.offset = offset;
                    }
                }/*lock scope*/

                resultsCv.notify_one();
            };

            // function to return the next number in the sequence
            auto nextValLambda = [&](ssize_t& nextVal){
                nextVal = counter.fetch_add(1);
                return nextVal < to;
            };

            // start the process
            rangeWorker.ProcessSequence(processLambda, nextValLambda);

            // wait for all the values to be processed
            std::unique_lock<std::mutex> lock(resultsMutex);
            resultsCv.wait(lock, [&](){
                return numProcessed == totalIterations;
            });

            return highest;
        }

        Offsetting::Confidence Offsetting::HighestValue(const QubitList& truth, const std::vector<SlotID>& validSlots, const QubitList& irregular, ssize_t from, ssize_t to)
        {
            std::mutex resultsMutex;
            std::condition_variable resultsCv;
            // counter to provide the next value in the sequence
            std::atomic_long counter(from);

            // result
            Confidence highest {0.0, 0};
            // the number of times the process will run
            const size_t totalIterations = to - from + 1;
            // the number of times the process has run
            size_t numProcessed = 0;

            // action to perform on every iteration, compare the values with an offset
            auto processLambda = [&](ssize_t offset){
                double confidence = CompareValues(truth, validSlots, irregular, offset);

                /*lock scope*/ {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    numProcessed++;
                    // store the value if it's the highest
                    if(confidence > highest.value)
                    {
                        highest.value = confidence;
                        highest.offset = offset;
                    }
                }/*lock scope*/

                resultsCv.notify_one();
            };

            // function to return the next number in the sequence
            auto nextValLambda = [&](ssize_t& nextVal){
                nextVal = counter.fetch_add(1);
                return nextVal < to;
            };

            // start the process
            rangeWorker.ProcessSequence(processLambda, nextValLambda);

            // wait for all the values to be processed
            std::unique_lock<std::mutex> lock(resultsMutex);
            resultsCv.wait(lock, [&](){
                return numProcessed == totalIterations;
            });

            return highest;
        }

        double Offsetting::CompareValues(const QubitList& truth, const std::vector<uint64_t>& validSlots,
                                         const QubitList& irregular,
                                         ssize_t offset)
        {
            double confidence = 0.5;
            size_t basesMatched = 0;
            size_t validCount = 0;
            // the number of elements to skip each time
            const auto step = irregular.size() / std::max(1ul, samples);
            const auto numToCheck = irregular.size();

            // step through a sample of values.
            for(uint64_t index = 0; index < numToCheck; index = index + step)
            {
                const auto adjusted = offset + static_cast<ssize_t>(validSlots[index]);
                const auto& aliceQubit = truth[static_cast<size_t>(adjusted)];
                const auto& bobQubit = irregular[index];

                if(adjusted >= 0 && adjusted < static_cast<ssize_t>(truth.size()) &&
                   QubitHelper::Base(aliceQubit) == QubitHelper::Base(bobQubit))
                {
                    // its within the range and the basis sent matches the basis measured
                    basesMatched++;
                    if(aliceQubit == bobQubit)
                    {
                       validCount++;
                    }
                }

            } // for each sample

            confidence = static_cast<double>(validCount) / basesMatched;

            return confidence;
        }

        double Offsetting::CompareValues(const QubitsBySlot markers, const std::vector<uint64_t>& validSlots,
                                         const QubitList& irregular,
                                         ssize_t offset)
        {
            using namespace std;
            double confidence = 0.5;
            size_t basesMatched = 0;
            size_t validCount = 0;

            for(const auto& marker : markers)
            {
                const auto adjustedSlot = static_cast<ssize_t>(marker.first) - offset;
                if(adjustedSlot >= 0)
                {
                    // us a binary search to find the value
                    const auto bobIndex = lower_bound(validSlots.cbegin(), validSlots.cend(), adjustedSlot);
                    // this may return with a "close" match but not the value we're looking for
                    if(bobIndex != validSlots.end() && *bobIndex == static_cast<SlotID>(adjustedSlot))
                    {
                        if(QubitHelper::Base(marker.second) == QubitHelper::Base(irregular[*bobIndex]))
                        {
                            basesMatched++;
                            if(marker.second == irregular[*bobIndex])
                            {
                                validCount++;
                            } // if values match
                        } // if bases match
                    } // if index found
                } // if slot id valid

                if(samples > 0 && basesMatched > samples)
                {
                    // stop looking
                    break;
                }
            } // for markers

            confidence = static_cast<double>(validCount) / basesMatched;

            return confidence;
        }

    } // namespace align
} // namespace cqp
