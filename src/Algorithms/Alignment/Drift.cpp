#include "Drift.h"
#include <future>
#include "Algorithms/Alignment/Filter.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Maths.h"

namespace cqp {
    namespace align {

        constexpr PicoSeconds Drift::DefaultDriftSampleTime;

        Drift::Drift(const PicoSeconds& slotWidth, const PicoSeconds& txJitter, const PicoSeconds& driftSampleTime):
            slotWidth{slotWidth}, driftSampleTime{driftSampleTime}
        {
            using namespace std::chrono;
            driftBins = slotWidth / txJitter;
        }

        void Drift::Histogram(const DetectionReportList::const_iterator& start,
                       const DetectionReportList::const_iterator& end,
                       uint64_t numBins,
                       PicoSeconds windowWidth,
                       std::vector<uint64_t>& counts)
        {
            counts.resize(numBins, 0);
            const auto binWidth = windowWidth / numBins;
            // for each detection
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                using namespace std::chrono;
                const uint64_t bin = DivNearest((detection->time % windowWidth).count(), binWidth.count());
                counts[bin]++;
            }

        }


        double Drift::FindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd) const
        {
            using namespace std;
            using namespace std::chrono;

            const auto binsCentre = driftBins / 2;
            std::vector<uint64_t> histogram;
            /*LOGDEBUG("Look from " + to_string(sampleStart->time.count()) + " to " +
                     to_string(sampleEnd->time.count()) + " in " +
                     to_string(distance(sampleStart, sampleEnd)) + " samples");
*/

            histogram.resize(driftBins, 0);
            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, driftBins, slotWidth, histogram);

            // get the extents of the graphs to find the centre of the transmission.
            const auto maxIt = std::max_element(histogram.cbegin(), histogram.cend());

            double average = 0.0;
            const size_t peakOffset = static_cast<size_t>(distance(histogram.cbegin(), maxIt));
            size_t totalWeights = 0;

            for(auto index = 0u; index < driftBins; index++)
            {
                // virtually roll the graph so that the peak is in the centre
                const auto shiftedBin = driftBins - ((driftBins + peakOffset + binsCentre - index) % driftBins);
                // weighted agverage based on the counts by multiplying the bin count (height of the peak) by
                // the bin number (~time) to find the mean of the peak - we wan a fractional number not
                // just a bin.
                totalWeights += histogram[index];
                average += static_cast<double>(histogram[index]) * shiftedBin;
            }

            average /= totalWeights;
            // fix the peak offset
            average = average + binsCentre - peakOffset;

            //LOGDEBUG("Peak: " + to_string(result.count()));
            return average;
        } // FindPeak

        double Drift::Calculate(const DetectionReportList::const_iterator& start,
                                        const DetectionReportList::const_iterator& end) const
        {
            using namespace std;
            /*
             * Take just enough data to detect the signal over the noise
             * find the centre of the detection mass when the edge goes over 3db
             *  |    ,,
             *  |___|  |____
             *  |   '  '
             *  |..'    '...
             *  |_____________
             *      ^  ^
             * Adjust the peak to keep it centred
             */

            // split the input in a number of samples
            // the data will be split to the nearest slotwidth

            auto sampleStart = start;
            auto sampleEnd = end;
            size_t sampleIndex = 1;

            // this will produce a sawtooth graph, the number of peaks depends on how often the drift pushes the peak past a slot edge
            vector<future<double>> peakFutures;

             while(distance(sampleStart, end) > 1)
             {
                DetectionReport cutoff;
                cutoff.time = start->time + driftSampleTime * sampleIndex;
                // use the binary search to find the point in the data where the time is past our sample time limit
                if(!Filter::FindEdge<DetectionReport>(sampleStart, sampleEnd, cutoff, sampleEnd, [](const auto& left, const auto&right){
                   return left.time > right.time;
                }))
                {
                    LOGERROR("Failed to find the time slice, using the last element");
                }
                //LOGDEBUG("Searching for peak in " + to_string(distance(sampleStart, sampleEnd)) + " samples");

                auto findStart = sampleStart;
                auto findEnd = sampleEnd;
                peakFutures.push_back(async(launch::async, &Drift::FindPeak, this, move(findStart), move(findEnd)));

                // set the start of the next sample
                sampleStart = sampleEnd;
                sampleEnd = end;
                sampleIndex++;
            }

            /* store the values for the peaks so we can discover the shift/wraparound
             the sequential values may wrap around the slot width:
             |   /|        |   |\
             |  / |        |   | \
             | /  |        |   |  \
             |    |  /  or |\  |   \
             |    | /      | \ |
             |    |/       |  \|
             |_________    |_________
            */
            std::vector<double> peaks;
            peaks.reserve(peakFutures.size());

            // copy the values from the other threads and find the highest peak
            for(auto peakFutIndex = 0u; peakFutIndex < peakFutures.size(); peakFutIndex++)
            {
                const auto value = peakFutures[peakFutIndex].get();

                peaks.push_back(value);

            }

            const auto binTime = (chrono::duration_cast<SecondsDouble>(slotWidth).count() / driftBins);
            double minimum = peaks[0];
            double maximum = peaks[0];
            size_t slopeStart = 0;
            double drift = 0.0;
            uint numSlopes = 0;

            for(auto index = 0u; index < peaks.size(); index++)
            {
                if(peaks[index] < minimum)
                {
                    // new low point
                    minimum = peaks[index];
                }

                if(peaks[index] > maximum)
                {
                    // new high point
                    maximum = peaks[index];
                }
                auto nextIndex = (index + 1) % peaks.size();
                auto peakDiff = abs(peaks[index] - peaks[nextIndex]);
                if(peakDiff - minimum > peaks[index])
                {
                    // we found an edge in the sawtooth,
                    // from slope start to index repressents a slope (ether positive or negative)
                    // The indices for the peaks are distanace from the slot start to the peak in 0 to the number of bins
                    // a bin is (slotwith / bins) long in time
                    double slopeTime = static_cast<double>(index - slopeStart + 1) * chrono::duration_cast<SecondsDouble>(driftSampleTime).count();
                    // take the difference between the extremes and scale it by the time covered by the samples
                    // the slope height represents the drift per sample
                    double slopeHeight = (maximum - minimum) * binTime;
                    if(peaks[slopeStart] < peaks[nextIndex])
                    {
                        // The slope is positive, make the drift negative
                        drift *= -1.0;
                    }
                    // add it to the total which will be averaged later
                    drift += slopeHeight / slopeTime;
                    numSlopes++;

                    // reset for the next slope
                    minimum = peaks[nextIndex];
                    maximum = peaks[nextIndex];
                    slopeStart = nextIndex;
                }
            }

            drift /= numSlopes;

            return drift;
        } // CalculateDrift

    } // namespace align
} // namespace cqp
