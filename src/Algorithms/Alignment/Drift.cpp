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
            LOGDEBUG("Look from " + to_string(sampleStart->time.count()) + " to " +
                     to_string(sampleEnd->time.count()) + " in " +
                     to_string(distance(sampleStart, sampleEnd)) + " samples");

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

            //const auto binTime = duration_cast<AttoSeconds>(window) / bins;
            // values are in "bins" so will need to be translated to time
            //average = round(average * binTime.count());
            //AttoSecondOffset result{static_cast<ssize_t>(average)};

            //LOGDEBUG("Peak: " + to_string(result.count()));
            return average;
        } // FindPeak

        AttoSecondOffset Drift::Calculate(const DetectionReportList::const_iterator& start,
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

            vector<future<double>> peakFutures;

            do{
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
                //FindPeak(findStart, findEnd, driftBins, slotWidth);


                // set the start of the next sample
                sampleStart = sampleEnd;
                sampleEnd = end;
                sampleIndex++;
            } while(distance(sampleStart, end) > 1);

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

            auto maxPeakIndex = 0ul;
            // copy the values from the other threads and find the highest peak
            for(auto peakFutIndex = 0u; peakFutIndex < peakFutures.size(); peakFutIndex++)
            {
                const auto value = peakFutures[peakFutIndex].get();
                //if(value > 0)
                {
                    peaks.push_back(value);
                    if(value >= peaks[maxPeakIndex])
                    {
                        // store the iterator to the last element
                        maxPeakIndex = peaks.size() - 1;
                    }
                    LOGDEBUG("Peak: " + to_string(value));
                }

            }

            // TODO: a circular iterator would make this easier - would need to write one.

            // if the peak is at the end - there is no need to shift
            if(maxPeakIndex < peaks.size())
            {
                // look at the point before the max peak
                std::vector<double>::const_iterator previousPeak;
                if(maxPeakIndex != 0)
                {
                    previousPeak = peaks.cbegin() + maxPeakIndex - 1;
                } else
                {
                    // the max peak is the first element, look at the last element
                    previousPeak = peaks.cend() - 1;
                }
                // look at the value before and after the peak,
                // the smallest value tells us the direction of the slope
                if(peaks[maxPeakIndex + 1] < *previousPeak)
                {
                    // the value to the right is the smaller value
                    // the slope is positive, start to the right of the peak
                    maxPeakIndex += 1;
                } else {
                    // the value to the left is the smaller value
                    // the slope is negative, start at the peak
                }

            }

            auto averageCount = 0u;
            double average{0};

            for(size_t index = 1; index < peaks.size(); index++)
            {
                // get the indexes, wraping around the graph
                const auto previousPeak = (index - 1 + maxPeakIndex) % peaks.size();
                const auto thisPeak = (index + maxPeakIndex) % peaks.size();
                // calculate the change in time between the two peaks
                const auto peakDifference = peaks[thisPeak] - peaks[previousPeak];
                //if(peakDifference.count() > 0)
                {
                    averageCount++;
                    average += peakDifference;
                }
            }

            AttoSecondOffset result { static_cast<ssize_t>(
                            round(average * driftSampleTime.count() / averageCount))};

            return result;
        } // CalculateDrift

    } // namespace align
} // namespace cqp
