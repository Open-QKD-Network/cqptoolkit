#include "Drift.h"
#include <future>
#include "Algorithms/Alignment/Filter.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Maths.h"
#include "Algorithms/Util/ProcessingQueue.h"

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
                const uint64_t bin = (DivNearest((detection->time % windowWidth).count(), binWidth.count())) % numBins;
                counts[bin]++;
            }

        }

        void Drift::Histogram(const DetectionReportList::const_iterator& start,
                       const DetectionReportList::const_iterator& end,
                       uint64_t numBins,
                       PicoSeconds windowWidth,
                       std::vector<uint64_t>& counts,
                       uint8_t channel)
        {
            counts.resize(numBins, 0);
            const auto binWidth = windowWidth / numBins;
            // for each detection
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                if (detection->value == channel){
                    using namespace std::chrono;
                    const uint64_t bin = DivNearest((detection->time % windowWidth).count(), binWidth.count());
                    counts[bin]++;
                }
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

            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, driftBins, slotWidth, histogram);

            // get the extents of the graphs to find the centre of the transmission.
            const auto maxIt = std::max_element(histogram.cbegin(), histogram.cend());

            double average = 0.0;
            const size_t peakOffset = static_cast<size_t>(distance(histogram.cbegin(), maxIt));
            size_t totalWeights = 0;
            // shift the index so the peak is in the middle
            const ssize_t indexShift = binsCentre - peakOffset;

            for(auto index = 0u; index < driftBins; index++)
            {
                // virtually roll the graph so that the peak is in the centre, add one so the first bin number is one,
                // otherwise the fist bin wont count (*0)
                const auto shiftedBin = ((driftBins + indexShift + index) % driftBins) + 1;
                // weighted agverage based on the counts by multiplying the bin count (height of the peak) by
                // the bin number (~time) to find the mean of the peak - we want a fractional number not
                // just a bin.
                totalWeights += histogram[index];
                average += shiftedBin * static_cast<double>(histogram[index]);
            }

            average /= totalWeights;
            // fix the peak offset
            average = fmod(average + driftBins - indexShift - 1, driftBins) ;

            //LOGDEBUG("Peak: " + to_string(result.count()));
            return average;
        } // FindPeak

        std::vector<double> Drift::ChannelFindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd) const
        {
            using namespace std;
            using namespace std::chrono;

            const auto binsCentre = driftBins / 2;

            std::vector< std::vector<uint64_t> > channelHistograms(4);
            std::vector<double> channelCentres(4);
            int channel = 0;

            for(auto hist : channelHistograms){
                hist.resize(driftBins,0);
                Histogram(sampleStart, sampleEnd, driftBins, slotWidth, hist, channel);

                // get the extents of the graphs to find the centre of the transmission.
                const auto maxIt = std::max_element(hist.cbegin(), hist.cend());

                double average = 0.0;
                const size_t peakOffset = static_cast<size_t>(distance(hist.cbegin(), maxIt));
                size_t totalWeights = 0;

                for(auto index = 0u; index < driftBins; index++)
                {
                    // virtually roll the graph so that the peak is in the centre
                    const auto shiftedBin = driftBins - ((driftBins + peakOffset + binsCentre - index) % driftBins);
                    // weighted agverage based on the counts by multiplying the bin count (height of the peak) by
                    // the bin number (~time) to find the mean of the peak - we wan a fractional number not
                    // just a bin.
                    totalWeights += hist[index];
                    average += static_cast<double>(hist[index]) * shiftedBin;
                }

                average /= totalWeights;
                // fix the peak offset
                average = average + binsCentre - peakOffset;

                //LOGDEBUG("Peak: " + to_string(result.count()));
                channelCentres[channel] = average;
                channel++;
            }
            return channelCentres;
        } // FindPeak

        
        void Drift::GetPeaks(const DetectionReportList::const_iterator& start,
                      const DetectionReportList::const_iterator& end,
                      std::vector<double>& peaks, std::vector<double>::const_iterator& maximum) const
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

            {
                // this will produce a sawtooth graph, the number of peaks depends on how often the drift pushes the peak past a slot edge
                ProcessingQueue<double> workQueue
            #if defined(_DEBUG)
                            (1)
            #endif
                ;

                std::vector<std::future<double>> peakFutures;

                while(distance(sampleStart, end) > 1)
                {
                    DetectionReport cutoff;
                    cutoff.time = start->time + driftSampleTime * sampleIndex;
                    // use the binary search to find the point in the data where the time is past our sample time limit
                    Filter::FindThreshold<DetectionReport>(sampleStart, sampleEnd, cutoff, sampleEnd, [](const auto& left, const auto& right){
                                                          return left.time > right.time;
                    });

                    if(sampleEnd != (end - 1) || sampleEnd->time - sampleStart->time >= driftSampleTime)
                    {
                        peakFutures.emplace_back(
                                workQueue.Enqueue([&, sampleStart, sampleEnd]() {
                                    return FindPeak(move(sampleStart), move(sampleEnd));
                                })
                        );
                    }
                    //LOGDEBUG("Searching for peak in " + to_string(distance(sampleStart, sampleEnd)) + " samples");
                    // set the start of the next sample
                    sampleStart = sampleEnd;
                    sampleEnd = end;
                    sampleIndex++;
                }

                // set the size now to the max iterator doesn't get invalidated
                peaks.resize(peakFutures.size());
                maximum = peaks.end();
                auto index = 0u;

                for(auto& fut : peakFutures)
                {
                    peaks[index] = fut.get();
                    if(maximum == peaks.end() || peaks[index] > *maximum)
                    {
                        // new high point
                        maximum = peaks.begin() + index;
                    }
                    index++;
                }
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

        } // GetPeaks

        double Drift::Calculate(const DetectionReportList::const_iterator& start,
                                        const DetectionReportList::const_iterator& end) const
        {
            using namespace std;
            std::vector<double> peaks;

            std::vector<double>::const_iterator maximum = peaks.end();

            GetPeaks(start, end, peaks, maximum);

            double drift = 0.0;
            if(!peaks.empty())
            {
                const auto binTime = (chrono::duration_cast<SecondsDouble>(slotWidth).count() / driftBins);
                // find a single slope
                // the signal may have multiple slopes
                // | /|  /|  /|
                // |/ | / | / |
                // |  |/  |/  |/
                // |____________

                double slope = 0.0;
                size_t slopeSamples = 0;

                for(auto index = 0u; index < peaks.size() - 1; index++)
                {

                    auto nextIndex = index + 1;
                    auto peakDiff = peaks[nextIndex] - peaks[index];

                    if(abs(peakDiff) < *maximum / 2.0)
                    {
                        // we havn't hit an edge
                        slope += peakDiff;
                        slopeSamples++;
                    }
                }

                // last element
                drift = (slope * binTime) / (slopeSamples * chrono::duration_cast<SecondsDouble>(driftSampleTime).count());
            }

            return drift;
        } // CalculateDrift

    } // namespace align
} // namespace cqp
