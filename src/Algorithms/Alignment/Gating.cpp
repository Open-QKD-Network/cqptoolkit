#include "Gating.h"
#include <cmath>
#include "Algorithms/Logging/Logger.h"
#include <fstream>
#include "Algorithms/Alignment/Filter.h"
#include <future>

namespace cqp {
    namespace align {

        // This deifines the storage unit for the constexpr in the header
        // this looks weird because the constant is defined in the header
        constexpr PicoSeconds Gating::DefaultPulseWidth;
        constexpr PicoSeconds Gating::DefaultSlotWidth;

        Gating::Gating(const PicoSeconds& slotWidth, const PicoSeconds& pulseWidth, uint64_t slotsPerDriftSample, double acceptanceRatio):
            slotWidth{slotWidth}, pulseWidth{pulseWidth},
            slotsPerDriftSample{slotsPerDriftSample}, numBins{slotWidth / pulseWidth},
            acceptanceRatio{acceptanceRatio}
        {

        }

        void Gating::Histogram(const DetectionReportList::const_iterator& start,
                       const DetectionReportList::const_iterator& end,
                       Gating::CountsByBin& counts) const
        {
            counts.resize(numBins);
            // for each detection
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                using namespace std::chrono;
                const BinID bin = (detection->time % slotWidth) / pulseWidth;
                counts[bin]++;
            }

        }

        void Gating::CountDetections(const DetectionReportList::const_iterator& start,
                                    const DetectionReportList::const_iterator& end,
                                    Gating::CountsByBin& counts,
                                    ResultsByBinBySlot& slotResults) const
        {
            using namespace std;
            counts.resize(numBins);

            // for each detection
            //   calculate it's slot and bin ids and store a reference to the original data
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                using namespace std::chrono;
                // calculate the offset in whole picoseconds (signed)
                const PicoSecondOffset offset { static_cast<PicoSecondOffset::rep>(
                        drift.count() * static_cast<double>(detection->time.count()) / static_cast<double>(PicoSeconds::period::den)
                )};
                // offset the time without the original value being converted to a float
                const PicoSeconds adjustedTime = detection->time + offset;
                // C++ truncates on integer division
                const SlotID slot = adjustedTime / slotWidth;
                const BinID bin = (adjustedTime % slotWidth) / pulseWidth;
                // store the value as a bin for later access
                slotResults[bin][slot].push_back(detection->value);
                counts[bin]++;
            }

        } // CountDetections

        PicoSeconds Gating::FindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd) const
        {
            using namespace std;
            CountsByBin histogram;
            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, histogram);

            // get the extents of the graphs to find the centre of the transmission.
            auto findMin = std::async(std::launch::async, [&](){
                return *std::min_element(histogram.cbegin(), histogram.cend());
            });

            uint64_t maxValue = 0;
            CountsByBin::const_iterator maxIndex = histogram.begin();
            // find the value and index of the maxima
            for(auto it = histogram.cbegin(); it != histogram.cend(); it++)
            {
                if(*it > maxValue)
                {
                    maxValue = *it;
                    maxIndex = it;
                }
            }

            // the cutoff limit for our peak.
            const uint64_t halfPower = maxValue - findMin.get();

            // DANGER: This may fail due to local minima,
            // might need to smooth the values or at at least remove the local minima before/while finding the edges.
            auto findLeft = std::async(std::launch::async, [& histogram, halfPower](){
                auto leftEdge = histogram.begin();
                Filter::FindEdge<uint64_t>(histogram.begin(), histogram.end(), halfPower, leftEdge);
                return *leftEdge;
            });

            auto findRight = std::async(std::launch::async, [& histogram, halfPower](){
                auto rightEdge = histogram.end();
                Filter::FindEdge<uint64_t>(histogram.begin(), histogram.end(), halfPower, rightEdge, std::greater<uint64_t>());
                return *rightEdge;
            });

            // using the centre of the edges should avoid finding spurious spikes in the counts.
            // The left edge may be > than the right if the peak is shifted to the edge of the histogram
            const uint64_t leftEdge = findLeft.get();
            const uint64_t peakWidth = static_cast<uint64_t>(
                        abs(static_cast<int64_t>(findRight.get() - leftEdge))
            );
            const size_t peak = ((peakWidth / 2) + leftEdge) % numBins;
            // values are in "bins" so will need to be translated to time
            LOGDEBUG("Peak = " + to_string(peak));
            return peak * pulseWidth;
        }

        PicoSecondOffset Gating::CalculateDrift(const DetectionReportList::const_iterator& start,
                                        const DetectionReportList::const_iterator& end)
        {
            using namespace std;
            /*
             * Take just enough data to detect the signal over the noise
             * find the centre of the deteaction mass when the edge goes over 3db
             *  |    ,,
             *  |___|  |____
             *  |   '  '
             *  |..'    '...
             *  |_____________
             *      ^  ^
             * Adjust the peak to keep it centred
             */

            const auto driftSampleTime = slotWidth * slotsPerDriftSample;
            auto sampleStart = start;
            auto sampleEnd = end;
            size_t sampleIndex = 1;

            vector<future<PicoSeconds>> peakFutures;

            do{
                DetectionReport cutoff;
                cutoff.time = driftSampleTime * sampleIndex;
                // use the binary search to find the point in the data where the time is past our sample time limit
                if(!Filter::FindEdge<DetectionReport>(sampleStart, sampleEnd, cutoff, sampleEnd, [](const auto& left, const auto&right){
                   return left.time < right.time;
                }))
                {
                    LOGERROR("Failed to find the time slice, useing the last element");
                }

                peakFutures.push_back(async(launch::async, &Gating::FindPeak, this, sampleStart, sampleEnd));
                // set the start of the next sample
                sampleStart = sampleEnd;
            } while(sampleEnd != end);

            PicoSecondOffset result{0};

            // average the seperation between the peaks
            // TODO: weighted average?
            for(auto it = peakFutures.begin(); it != peakFutures.end(); it++)
            {
                result += (result + it->get()) / 2;
            }

            return result;
        }

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   const QubitsBySlot& markers,
                                   QubitList& results)
        {
            drift = CalculateDrift(start, end);
            LOGDEBUG("Drift = " + std::to_string(drift.count()));

        } // ScoreOffsets

    } // namespace align
} // namespace cqp
