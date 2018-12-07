#include "Filter.h"
#include "Algorithms/Util/DataFile.h"

namespace cqp {
    namespace align {

        Filter::Filter(double sigma, size_t filterWidth, double cutoff, size_t stride) :
                filter{Filter::GaussianWindow1D(sigma, filterWidth)},
                cutoff{cutoff}, stride{stride}
        {

        }

        void Filter::Isolate(const DetectionTimes& timeTags,
                             DetectionTimes::const_iterator& start,
                             DetectionTimes::const_iterator& end)
        {
            using namespace std;

            // difference the values
            vector<uint64_t> diffs;

            if(timeTags.size() > stride)
            {
                diffs.reserve(timeTags.size() / stride);
                // the first element of diffs will equal the first element of timetags
                for(auto tagIt = timeTags.begin() + stride; tagIt < timeTags.end(); tagIt += stride)
                {
                    const auto prev = tagIt - stride;
                    diffs.push_back(tagIt->count() - prev->count());
                }
            } else {
                LOGERROR("stride is wider than data");
            }

            // convolve the data to find the start of transmission
            vector<double> convolved;
            ConvolveValid(diffs.begin(), diffs.end(), filter.begin(), filter.end(), convolved);
            const auto minima = *min_element(convolved.begin(), convolved.end());
            const auto maxima = *max_element(convolved.begin(), convolved.end());

            const auto cutoffScaled = (cutoff + minima) * maxima;
            // find the start and end of the transmission
            const auto convStart = upper_bound(convolved.cbegin(), convolved.cend(), cutoffScaled);
            const auto convEnd = upper_bound(convolved.crbegin(), convolved.crend(), cutoffScaled).base();

            // store the offsets we've calculated
            start = timeTags.begin() + distance(convolved.cbegin(), convStart) * stride;
            end = timeTags.begin() + distance(convolved.cbegin(), convEnd) * stride;

        }

    } // namespace align
} // namespace cqp
