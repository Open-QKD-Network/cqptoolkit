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
        constexpr PicoSeconds Gating::DefaultTxJitter;
        constexpr PicoSeconds Gating::DefaultSlotWidth;
        constexpr PicoSeconds Gating::DefaultMaxDrift;

        template<typename T, typename D>
        T div_to_nearest(T n, D d) {
          if (n < T(0)) {
            return (n - (d/2) + 1) / d;
          } else {
            return (n + d/2) / d;
          }
        }

        Gating::Gating(std::shared_ptr<IRandom> rng,
                       uint64_t slotsPerFrame,
                       const PicoSeconds& slotWidth, const PicoSeconds& txJitter,
                       const PicoSeconds& maxDrift, double acceptanceRatio):
            rng(rng),
            slotsPerFrame{slotsPerFrame},
            slotWidth{slotWidth}, txJitter{txJitter},
            maxDrift{maxDrift},
            numBins{slotWidth / txJitter},
            acceptanceRatio{acceptanceRatio}
        {
            using namespace std::chrono;

            const size_t slotsPerSecond = (seconds(1) / slotWidth);
            const auto samplesPerTest = div_to_nearest(slotsPerSecond, maxDrift.count());
            driftSampleTime = slotWidth * samplesPerTest * 10000;
            //driftSampleTime = (slotsPerFrame * slotWidth) / 40;
            driftBins = slotWidth / txJitter;
        }

        void Gating::Histogram(const DetectionReportList::const_iterator& start,
                       const DetectionReportList::const_iterator& end,
                       uint64_t numBins,
                       PicoSeconds windowWidth,
                       Gating::CountsByBin& counts)
        {
            counts.resize(numBins, 0);
            const auto binWidth = windowWidth / numBins;
            // for each detection
            //   count the bin ids
            for(auto detection = start; detection < end; ++detection)
            {
                using namespace std::chrono;
                const BinID bin = div_to_nearest((detection->time % windowWidth).count(), binWidth.count());
                counts[bin]++;
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

            //const double scaledDrift = (drift * PicoSecondOffset::period::den).count();
            // for each detection
            //   calculate it's slot and bin ids and store a reference to the original data
            //   count the bin ids
            for(auto detection = start; detection != end; ++detection)
            {
                using namespace std::chrono;
                // calculate the offset in whole picoseconds (signed)
                //const PicoSecondOffset offset { static_cast<int64_t>(
                //    (scaledDrift * duration_cast<SecondsDouble>(detection->time)).count())
                //};
                const PicoSecondOffset offset { div_to_nearest((detection->time * duration_cast<PicoSeconds>(drift).count()).count(), PicoSecondOffset::period::den) };
                // offset the time without the original value being converted to a float
                const PicoSeconds adjustedTime = detection->time - frameStart - offset;
                // C++ truncates on integer division
                const SlotID slot = div_to_nearest(adjustedTime.count(), slotWidth.count());
                //const SlotID slot = adjustedTime.count() / slotWidth.count();
                // through away anything past the end of transmission
                //if(slot < slotsPerFrame)
                {
                    const auto fromSlotStart = adjustedTime % slotWidth;
                    //const BinID bin = div_to_nearest(fromSlotStart.count(), pulseWidth.count()) % numBins;
                    const BinID bin = (fromSlotStart.count()/ txJitter.count()) % numBins;
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

            LOGDEBUG(" lower=" + to_string(lower) + "Peak=" + to_string(peakIndex) + " upper=" + to_string(upper));
            map<SlotID, QubitList> qubitsBySlot;
            // walk through each bin, wrapping around to the start if the upper bin < lower bin

            size_t binCount = 0;

            std::ofstream datafile = std::ofstream("gated-binned.csv");
            datafile << "SlotID, QubitValue, BinID, Ordinal" << std::endl;
            for(auto binId = (lower + 1) % numBins; binId != upper; binId = (binId + 1) % numBins)
            {
                SlotID slotOffset = 0;
                // If the bin ID is less than the lower limit we're left of bin 0
                if(upper < lower && binId < upper)
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

        double Gating::FindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd, size_t bins, PicoSeconds window)
        {
            using namespace std;
            using namespace std::chrono;

            const auto binsCentre = bins / 2;
            CountsByBin histogram;
            LOGDEBUG("Look from " + to_string(sampleStart->time.count()) + " to " +
                     to_string(sampleEnd->time.count()) + " in " +
                     to_string(distance(sampleStart, sampleEnd)) + " samples");

            histogram.resize(bins, 0);
            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, bins, window, histogram);

            //histogram = Filter::MovingAverage(histogram, 6);
            // get the extents of the graphs to find the centre of the transmission.
            const auto maxIt = std::max_element(histogram.cbegin(), histogram.cend());

            double average = 0.0;
            const auto peakOffset = distance(histogram.cbegin(), maxIt);
            size_t totalWeights = 0;

            for(auto index = 0u; index < bins; index++)
            {
                // virtually roll the graph so that the peak is in the centre
                const auto shiftedBin = bins - ((bins + peakOffset + binsCentre - index) % bins);
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

        AttoSecondOffset Gating::CalculateDrift(const DetectionReportList::const_iterator& start,
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
                peakFutures.push_back(async(launch::async, &Gating::FindPeak, move(findStart), move(findEnd), driftBins, slotWidth));
                //FindPeak(findStart, findEnd, driftBins, slotWidth);


                // set the start of the next sample
                sampleStart = sampleEnd + 1;
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

            auto maxPeakIndex = 0u;
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

            {
                std::ofstream datafile = ofstream("drift.csv");
                for(auto index = 0u; index < peaks.size(); index++)
                {
                    datafile << std::to_string(peaks[index]) << endl;
                }
                datafile.close();
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

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   ValidSlots& validSlots,
                                   QubitList& results,
                                   bool calculateDrift)
        {
            using namespace std::chrono;
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

            LOGDEBUG("Drift = " + std::to_string(duration_cast<PicoSecondOffset>(drift).count()) + "ps");
            LOGDEBUG("Peak width: " + std::to_string(peakWidth * 100.0) + "%");


        }

    } // namespace align
} // namespace cqp
