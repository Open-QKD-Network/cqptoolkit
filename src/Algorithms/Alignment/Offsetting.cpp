#include "Offsetting.h"

namespace cqp {
    namespace align {

        Offsetting::Offsetting(size_t samples,
                               double rejectionThreshold,
                               size_t minTests):
            rejectionThreshold(rejectionThreshold),
            minTests(minTests),
            samples(samples)
        {

        }

        double Offsetting::CompareValues(const QubitList& truth, const std::vector<uint64_t>& validSlots,
                                         const QubitList& irregular,
                                         ssize_t offset)
        {
            double confidence = 0.5;
            size_t basesMatched = 0;
            size_t validCount = 0;
            const auto step = irregular.size() / samples;
            const auto numToCheck = irregular.size();

            for(uint64_t index = 0; index < numToCheck; index = index + step)
            {
                const auto adjusted = offset + static_cast<ssize_t>(validSlots[index]);
                const auto& aliceQubit = truth[static_cast<size_t>(adjusted)];
                const auto& bobQubit = irregular[index];

                if(adjusted >= 0 && adjusted < static_cast<ssize_t>(truth.size()) &&
                   QubitHelper::Base(aliceQubit) == QubitHelper::Base(bobQubit))
                {
                    basesMatched++;
                    if(aliceQubit == bobQubit)
                    {
                       validCount++;
                    }
                }

                if(basesMatched > minTests)
                {
                    confidence = static_cast<double>(validCount) / basesMatched;
                    if(confidence < rejectionThreshold)
                    {
                        break;
                    }
                }
            }

            return confidence;
        }

    } // namespace align
} // namespace cqp
