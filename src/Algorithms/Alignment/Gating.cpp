#include "Gating.h"
#include <cmath>
#include "Algorithms/Logging/Logger.h"
#include <fstream>
#include "Algorithms/Alignment/Filter.h"
#include <future>
#include <fstream>
namespace cqp {
    namespace align {

        // This deifines the storage unit for the constexpr in the header
        // this looks weird because the constant is defined in the header
        constexpr PicoSeconds Gating::DefaultPulseWidth;
        constexpr PicoSeconds Gating::DefaultSlotWidth;
        constexpr PicoSeconds Gating::DefaultDriftResolution;
        constexpr PicoSeconds Gating::DefaultMaxDrift;

        Gating::Gating(std::shared_ptr<IRandom> rng,
                       uint64_t slotsPerFrame,
                       const PicoSeconds& slotWidth, const PicoSeconds& pulseWidth,
                       const PicoSeconds& driftResolution, const PicoSeconds& maxDrift,
                       double acceptanceRatio):
            rng(rng),
            slotsPerFrame{slotsPerFrame},
            slotWidth{slotWidth}, pulseWidth{pulseWidth},
            driftResolution{driftResolution}, maxDrift{maxDrift},
            numBins{slotWidth / pulseWidth},
            acceptanceRatio{acceptanceRatio}
        {
            using namespace std::chrono;
            const size_t slotsPerSecond = (seconds(1) / slotWidth);
            const double maxDriftPerSlot = static_cast<double>(maxDrift.count()) / slotsPerSecond;
            const size_t slotsPerSample = slotWidth.count() / maxDriftPerSlot;
            driftSampleTime = slotWidth * slotsPerSample;
            driftBins = slotWidth / driftResolution;
        }

        void Gating::Histogram(const DetectionReportList::const_iterator& start,
                       const DetectionReportList::const_iterator& end,
                       uint64_t numBins,
                       PicoSeconds windowWidth,
                       Gating::CountsByBin& counts)
        {
            counts.resize(numBins, 0);
            const double binWidth = static_cast<double>(windowWidth.count()) / numBins;
            // for each detection
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                using namespace std::chrono;
                const BinID bin = static_cast<uint64_t>((detection->time % windowWidth).count() / binWidth) % numBins;
                counts[bin]++;
            }

        }

        template<typename T>
        T div_to_nearest(T n, T d) {
          if (n < T(0)) {
            return (n - (d/2) + 1) / d;
          } else {
            return (n + d/2) / d;
          }
        }

        void Gating::CountDetections(const PicoSeconds& frameStart,
                                     const DetectionReportList::const_iterator& start,
                                     const DetectionReportList::const_iterator& end,
                                     Gating::CountsByBin& counts,
                                     ResultsByBinBySlot& slotResults) const
        {
            using namespace std;
            counts.resize(numBins, 0);
            slotResults.resize(numBins);

            const double scaledDrift = (drift * PicoSecondOffset::period::den).count();
            // for each detection
            //   calculate it's slot and bin ids and store a reference to the original data
            //   count the bin ids
            for(auto detection = start; detection != end; ++detection)
            {
                using namespace std::chrono;
                // calculate the offset in whole picoseconds (signed)
                const PicoSecondOffset offset { static_cast<int64_t>(
                    (scaledDrift * duration_cast<SecondsDouble>(detection->time)).count())
                };
                // offset the time without the original value being converted to a float
                const PicoSeconds adjustedTime = detection->time - frameStart - offset;
                // C++ truncates on integer division
                const SlotID slot = div_to_nearest(adjustedTime.count(), slotWidth.count());
                // through away anything past the end of transmission
                //if(slot < slotsPerFrame)
                {
                    const auto fromSlotStart = adjustedTime % slotWidth;
                    const BinID bin = div_to_nearest(fromSlotStart.count(), pulseWidth.count()) % numBins;
                    // store the value as a bin for later access
                    slotResults[bin][slot].push_back(detection->value);
                    counts[bin]++;
                }
            }

        }

        void Gating::GateResults(const Gating::CountsByBin& counts, const Gating::ResultsByBinBySlot& slotResults,
                                 ValidSlots& validSlots, QubitList& results, double* peakWidth) const
        {
            using namespace std;
            // find the peak from the counts
            const size_t peakIndex = static_cast<size_t>(distance(counts.cbegin(), max_element(counts.cbegin(), counts.cend())));
            const auto minValue = *min_element(counts.cbegin(), counts.cend());
            const auto cutoff = static_cast<size_t>(minValue + (counts[peakIndex] - minValue) * acceptanceRatio);
            auto upper = peakIndex;
            auto lower = peakIndex;

            while(counts[lower] > cutoff && lower != (peakIndex + 1) % numBins)
            {
                lower = (numBins + lower - 1) % numBins;
            }

            while(counts[upper] > cutoff && upper != (peakIndex - 1) % numBins)
            {
                upper = (upper + 1) % numBins;
            }

            LOGDEBUG("Peak=" + to_string(peakIndex) + " upper=" + to_string(upper) + " lower=" + to_string(lower));
            map<SlotID, QubitList> qubitsBySlot;
            // walk through each bin, wrapping around to the start if the upper bin < lower bin

            size_t binCount = 0;

            std::ofstream datafile = std::ofstream("gated-binned.csv");
            datafile << "SlotID, QubitValue, BinID, Ordinal" << std::endl;
            for(auto binId = (lower + 1) % numBins; binId != upper ; binId = (binId + 1) % numBins)
            {
                SlotID slotOffset = 0;
                // If the bin ID is less than the lower limit we're left of bin 0
                if(upper < lower && binId > lower)
                {
                    // the peak wraps around, adjust the slot id for bins to the left
                    slotOffset = 1;
                }
                binCount++;
                for(auto slot : slotResults[binId])
                {
                    const auto mySlot = slot.first + slotOffset;
                    // add the qubits to the list for this slot, one will be chosen at random later
                    qubitsBySlot[mySlot].insert(qubitsBySlot[mySlot].end(), slot.second.cbegin(), slot.second.cend());
                    int ordinal = 0;
                    for(auto val : slot.second)
                    {
                        datafile << std::to_string(slot.first) << ", " <<
                                    std::to_string(val) << ", "
                                 << std::to_string(binId) << ", "
                                 << std::to_string(ordinal) << std::endl;
                        ordinal++;
                    }
                } // for each slot
            } // for each bin

            datafile.close();

            if(peakWidth)
            {
                *peakWidth = (binCount / static_cast<double>(numBins));
            }

            size_t multiSlots = 0;

            // as the list is ordered, the qubits will come out in the correct order
            // just append them to the result list
            for(auto list : qubitsBySlot)
            {
                if(!list.second.empty())
                {
                    // record that we have a value for this slot
                    validSlots.push_back(list.first);
                    if(list.second.size() == 1)
                    {
                        results.push_back(list.second[0]);
                    }
                    else {
                        multiSlots++;
                        //LOGDEBUG("Multiple qubits for slot");
                        // pick a qubit at random
                        const auto index = rng->RandULong() % list.second.size();
                        results.push_back(list.second[index]);
                    }
                }
            }

            LOGDEBUG("Number of multi-qubit slots: " + to_string(multiSlots));

            // results now contains a contiguous list of qubits
            // validSlots tells the caller which slots were used to create that list.
        } // CountDetections

        PicoSeconds Gating::FindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd) const
        {
            using namespace std;
            CountsByBin histogram;
            LOGDEBUG("Look from " + to_string(sampleStart->time.count()) + " to " +
                     to_string(sampleEnd->time.count()) + " in " +
                     to_string(distance(sampleStart, sampleEnd)) + " samples");

            histogram.resize(driftBins, 0);
            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, numBins, slotWidth, histogram);

            histogram = Filter::MovingAverage(histogram, 6);
            // get the extents of the graphs to find the centre of the transmission.
            const auto maxIt = std::max_element(histogram.cbegin(), histogram.cend());

            PicoSeconds result { 0 };
            if(maxIt != histogram.end() && *maxIt > 0)
            {
                // values are in "bins" so will need to be translated to time
                result = distance(histogram.cbegin(), maxIt) * driftResolution;
            }

            LOGDEBUG("Peak: " + to_string(result.count()));
            return result;
        } // FindPeak

        SecondsDouble Gating::CalculateDrift(const DetectionReportList::const_iterator& start,
                                        const DetectionReportList::const_iterator& end) const
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

            // split the input in a number of samples
            // the data will be split to the nearest slotwidth

            auto sampleStart = start;
            auto sampleEnd = end;
            size_t sampleIndex = 1;

            vector<future<PicoSeconds>> peakFutures;

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
                peakFutures.push_back(async(launch::async, &Gating::FindPeak, this, move(findStart), move(findEnd)));

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
            std::vector<PicoSecondOffset> peaks;
            peaks.reserve(peakFutures.size());

            auto maxPeakIndex = 0u;
            // copy the values from the other threads and find the highest peak
            for(auto peakFutIndex = 0u; peakFutIndex < peakFutures.size(); peakFutIndex++)
            {
                const auto value = peakFutures[peakFutIndex].get();
                if(value.count() > 0)
                {
                    peaks.push_back(value);
                    if(value >= peaks[maxPeakIndex])
                    {
                        // store the iterator to the last element
                        maxPeakIndex = peaks.size() - 1;
                    }
                    LOGDEBUG("Peak: " + to_string(value.count()));
                }

            }

            // TODO: a circular iterator would make this easier - would need to write one.

            // if the peak is at the end - there is no need to shift
            if(maxPeakIndex < peaks.size())
            {
                // look at the point before the max peak
                std::vector<PicoSecondOffset>::const_iterator previousPeak;
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
            SecondsDouble average{0};
            const SecondsDouble sampleTimeSeconds = chrono::duration_cast<SecondsDouble>(driftSampleTime);
            for(size_t index = 1; index < peaks.size(); index++)
            {
                // get the indexes, wraping around the graph
                const auto previousPeak = (index - 1 + maxPeakIndex) % peaks.size();
                const auto thisPeak = (index + maxPeakIndex) % peaks.size();
                // calculate the change in time between the two peaks
                const auto peakDifference = peaks[thisPeak] - peaks[previousPeak];
                if(peakDifference.count() > 0)
                {
                    averageCount++;
                    average += peakDifference / sampleTimeSeconds.count();
                }
            }

            //const auto slotsPerSecond = chrono::duration_cast<PicoSeconds>(chrono::seconds(1)) / slotWidth;
            average = average  / averageCount;

            return average;
        } // CalculateDrift

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   ValidSlots& validSlots,
                                   QubitList& results,
                                   bool calculateDrift)
        {
            if(calculateDrift)
            {
                drift = CalculateDrift(start, end);
            }
            CountsByBin counts;
            ResultsByBinBySlot resultsBySlot;
            CountDetections(start->time, start, end, counts, resultsBySlot);

            double peakWidth = 0.0;
            GateResults(counts, resultsBySlot, validSlots, results, &peakWidth);

            stats.PeakWidth.Update(peakWidth);
            stats.Drift.Update(drift);

            LOGDEBUG("Drift = " + std::to_string(drift.count() * 1.0e9 ) + "ns");
            LOGDEBUG("Peak width: " + std::to_string(peakWidth * 100.0) + "%");


        }

    } // namespace align
} // namespace cqp
