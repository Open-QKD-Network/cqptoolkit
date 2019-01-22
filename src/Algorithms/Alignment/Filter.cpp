#include "Filter.h"
#include "Algorithms/Logging/Logger.h"

namespace cqp {
    namespace align {

        Filter::Filter(double sigma, size_t filterWidth, double courseThreshold, double fineThreshold, size_t initialStride) :
                filter{Filter::GaussianWindow1D(sigma, filterWidth)},
                courseThreshold{courseThreshold}, fineThreshold{fineThreshold},
                initialStride{initialStride}
        {

        }

        bool Filter::Isolate(const std::vector<double>& filter, size_t stride, double threshold, bool findStart,
                             DetectionReportList::const_iterator begin, DetectionReportList::const_iterator end,
                          IteratorPair& edgeRange)
        {
            bool result = false;
            using namespace std;
            // set defaults for error condition
            edgeRange.first = begin;
            edgeRange.second = end;
            const auto numElements = static_cast<size_t>(abs(distance(begin, end)));


            if(numElements > stride)
            {
                auto prevTagIt = begin;

                // difference the values
                vector<uint64_t> diffs;
                diffs.reserve(numElements / stride);
                // the first element of diffs will equal the first element of timetags
                for(auto tagIt = begin + static_cast<ssize_t>(stride);
                    tagIt < end;
                    tagIt += static_cast<ssize_t>(stride))
                {
                    if(tagIt < end)
                    {
                        diffs.push_back(tagIt->time.count() - prevTagIt->time.count());
                    }
                    // this tag now become the previous for the next iteration
                    prevTagIt = tagIt;
                }
                // convolve the data to find the start of transmission
                vector<uint64_t> convolved;
                //convolved = MovingAverage(diffs, 10);
                if(ConvolveValid(diffs.begin(), diffs.end(), filter.begin(), filter.end(), convolved))
                {
                    const auto minima = *min_element(convolved.begin(), convolved.end());
                    const auto maxima = *max_element(convolved.begin(), convolved.end());

                    if(maxima > minima)
                    {
                        // dont bother finding an edge when the diff is flat
                        const uint64_t cutoffScaled = static_cast<uint64_t>(maxima * threshold) + minima;
                        auto edge = convolved.cbegin();
                        if(findStart)
                        {
                            while(edge != convolved.cend() && *edge > cutoffScaled)
                            {
                                edge++;
                            }
                            result = true; //FindEdge(convolved.cbegin(), convolved.cend(), cutoffScaled, edge);
                        } else {
                            auto revEdge = convolved.crbegin();
                            while(revEdge != convolved.crend() && *revEdge > cutoffScaled)
                            {
                                revEdge++;
                            }
                            edge = revEdge.base();
                            result = true; //result = FindEdge<double>(convolved.cbegin(), convolved.cend(), cutoffScaled, edge, std::greater<double>());
                        }

                        if(result)
                        {
                            // compensate for the shift caused by differencing and convolution
                            const auto edgeOffset = (distance(convolved.cbegin(), edge) + (filter.size() / 2 - 1)) * stride;
                            // store the offsets we've calculated
                            // the convolution process reduces the width of the graph, losing the rightmost edge
                            edgeRange.first = begin + (edgeOffset - stride + 1);
                            edgeRange.second = begin + (edgeOffset + stride - 1);
                        }
                    }
                }// else {
                //    LOGERROR("Convolution failed");
                //}
            } else {
                LOGERROR("stride is wider than data");
            }

            return result;
        }

        bool Filter::Isolate(const DetectionReportList& timeTags,
                             DetectionReportList::const_iterator& start,
                             DetectionReportList::const_iterator& end) const
        {
            using namespace std;
            bool result = false;

            IteratorPair startEdgeRange;
            // Look for the rough area where the window starts
            result = Isolate(filter, initialStride, courseThreshold, true, timeTags.begin(), timeTags.end(), startEdgeRange);
            LOGDEBUG("Course start: " + to_string(distance(timeTags.cbegin(), startEdgeRange.first)) + "(" + to_string(startEdgeRange.first->time.count()) + " pS)"
                     " to " + to_string(distance(timeTags.cbegin(), startEdgeRange.second)) + "(" + to_string(startEdgeRange.second->time.count()) + " pS)");
            if(result)
            {
                // Repeat the process with a fine grain approach within that window
                result = Isolate(filter, 1, fineThreshold, true, startEdgeRange.first, startEdgeRange.second + 1000, startEdgeRange);
                start = startEdgeRange.first;
            }

            IteratorPair endEdgeRange;
            // Look for the rough area where the window ends, starting from the send of the window start
            result = Isolate(filter, initialStride, courseThreshold, false, startEdgeRange.second, timeTags.end(), endEdgeRange);
            if(result)
            {
                // Repeat the process with a fine grain approach within that window
                result = Isolate(filter, 1, fineThreshold, false, endEdgeRange.first, endEdgeRange.second, endEdgeRange);
                end = endEdgeRange.first;
            }
            return result;
        }

    } // namespace align
} // namespace cqp
