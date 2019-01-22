#include "Offsetting.h"

namespace cqp {
    namespace align {

        Offsetting::Offsetting(size_t samples,
                               double rejectionThreshold,
                               double acceptanceThreshold,
                               size_t minTests):
            rejectionThreshold(rejectionThreshold),
            acceptanceThreshold(acceptanceThreshold),
            minTests(minTests),
            samples(samples)
        {

        }

        double Offsetting::CompareValues(const QubitList& truth, const std::vector<uint64_t>& validSlots,
                                         const QubitList& irregular,
                                         ssize_t offset)
        {
            using namespace std;
            double confidence = 0.5;
            size_t basesMatched = 0;
            size_t validCount = 0;

            const auto smallestList = min(truth.size(), irregular.size());
            const auto actualSamples = min(smallestList, samples);
            const size_t step = smallestList / actualSamples;

            const size_t indexEnd = static_cast<size_t>(
                        max(0l, min(static_cast<ssize_t>(truth.size()), static_cast<ssize_t>(irregular.size()) - offset))
                        );
            const size_t indexStart = static_cast<size_t>(0 - min(0l, offset)); // offset the start if offset is negative

            for(auto index = indexStart; index < indexEnd; index += step)
            //for(auto slotId : validSlots)
            {
                const size_t irregIndex = index + offset;
                if(QubitHelper::Base(truth[index]) == QubitHelper::Base(irregular[irregIndex]))
                {
                    basesMatched++;
                    if(truth[index] == irregular[irregIndex])
                    {
                        validCount++;
                    }

                    confidence = static_cast<double>(validCount) / basesMatched;
                    if(basesMatched > minTests && (confidence < rejectionThreshold || confidence > acceptanceThreshold))
                    {
                        break; // for
                    }
                }
            }

            return confidence;
        }

    } // namespace align
} // namespace cqp
