#include "Filter.h"
#include "Algorithms/Util/DataFile.h"

namespace cqp {
    namespace align {

        Filter::Filter(double sigma, size_t filterWidth, double cutoff, size_t stride) :
                filter{Filter::GaussianWindow1D(sigma, filterWidth)},
                cutoff{cutoff}, stride{stride}
        {

        }

        void Filter::Isolate(const DetectionReportList& timeTags,
                             DetectionReportList::const_iterator& start,
                             DetectionReportList::const_iterator& end)
        {
            using namespace std;

            // difference the values
            vector<uint64_t> diffs;

            if(timeTags.size() > stride)
            {
                diffs.reserve(timeTags.size() / stride);
                // the first element of diffs will equal the first element of timetags
                for(auto tagIt = timeTags.begin() + static_cast<ssize_t>(stride);
                    tagIt < timeTags.end();
                    tagIt += static_cast<ssize_t>(stride))
                {
                    const auto prev = tagIt - static_cast<ssize_t>(stride);
                    diffs.push_back(tagIt->time.count() - prev->time.count());
                }
            } else {
                LOGERROR("stride is wider than data");
            }

            fs::DataFile::WriteCSV("diffs2.csv", diffs.begin(), diffs.end(), "\n");

            // convolve the data to find the start of transmission
            vector<double> convolved;
            ConvolveValid(diffs.begin(), diffs.end(), filter.begin(), filter.end(), convolved);
            const auto minima = *min_element(convolved.begin(), convolved.end());
            const auto maxima = *max_element(convolved.begin(), convolved.end());

            const auto cutoffScaled = (maxima * cutoff) + minima;
            auto leftEdge = convolved.cbegin();
            auto rightEdge = convolved.cend();
            FindEdge(convolved.cbegin(), convolved.cend(), cutoffScaled, leftEdge);
            FindEdge<double>(leftEdge, convolved.cend(), cutoffScaled, rightEdge, std::greater<double>());

            LOGINFO("Left edge: " + to_string(distance(convolved.cbegin(), leftEdge)) +
                    " Right edge: " + to_string(distance(convolved.cbegin(), rightEdge)));

            // store the offsets we've calculated
            start = timeTags.cbegin() + distance(convolved.cbegin(), leftEdge) * static_cast<ssize_t>(stride);
            end = timeTags.cbegin() + distance(convolved.cbegin(), rightEdge) * static_cast<ssize_t>(stride);

        }

    } // namespace align
} // namespace cqp
