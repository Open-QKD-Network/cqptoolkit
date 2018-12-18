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

        Gating::Gating(std::shared_ptr<IRandom> rng,
                       const PicoSeconds& slotWidth, const PicoSeconds& pulseWidth, uint64_t slotsPerDriftSample, double acceptanceRatio):
            rng(rng),
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

        }

        void Gating::GateResults(const Gating::CountsByBin& counts, const Gating::ResultsByBinBySlot& slotResults,
                                 ValidSlots& validSlots, QubitList& results)
        {
            using namespace std;
            // find the peak from the counts
            const auto peak = max(counts.cbegin(), counts.cend());
            const auto minValue = *min(counts.cbegin(), counts.cend());
            const auto cutoff = (*peak - minValue) * acceptanceRatio;
            auto upper = peak;
            auto lower = peak;

            for(auto it = peak; it != counts.cend(); it++)
            {
                if(*it < cutoff)
                {
                    upper = it;
                }
            }
            if(upper == counts.cend())
            {
                // the edge has wrapped to the other side of the graph
                // keep looking from the start
                for(auto it = counts.begin(); it != peak; it++)
                {
                    if(*it < cutoff)
                    {
                        upper = it;
                    }
                }
            }

            for(auto it = make_reverse_iterator(peak); it != counts.crend(); it++)
            {
                if(*it < cutoff)
                {
                    lower = it.base();
                }
            }

            if(upper == counts.cbegin())
            {
                // the edge has wrapped to the other side of the graph
                // keep looking from the end
                for(auto it = counts.crend(); it != make_reverse_iterator(peak); it++)
                {
                    if(*it < cutoff)
                    {
                        lower = it.base();
                    }
                }
            }

            // for each usable bin, extract the values to pass to the next stage
            const BinID lowerBin = distance(counts.cbegin(), lower);
            const BinID upperBin = distance(counts.cbegin(), upper);

            map<SlotID, QubitList> qubitsBySlot;
            // walk through each bin, wrapping around to the start if the upper bin < lower bin
            const auto lastBin = (upperBin + 1) % numBins;

            for(auto binId = lowerBin; binId != lastBin ; binId = (binId + 1) % numBins)
            {
                const auto& slotsForBin = slotResults.at(binId);
                for(auto slot : slotsForBin)
                {
                    // record that we have a value for this slot
                    validSlots.insert(slot.first);
                    // add the qubits to the list for this slot, one will be chosen at random later
                    qubitsBySlot[slot.first].assign(slot.second.cbegin(), slot.second.cend());
                }
            }

            // as the list is ordered, the qubits will come out in the correct order
            // just append them to the result list
            for(auto list : qubitsBySlot)
            {
                if(list.second.size() == 1)
                {
                    results.push_back(list.second[0]);
                } else {
                    // pic a qubit at random
                    const auto index = rng->RandULong() % list.second.size();
                    results.push_back(list.second[index]);
                }
            }

            // results now contains a contiguous list of qubits
            // validSlots tells the caller which slots were used to create that list.
        } // CountDetections

        PicoSeconds Gating::FindPeak(DetectionReportList::const_iterator sampleStart,
                      DetectionReportList::const_iterator sampleEnd) const
        {
            using namespace std;
            CountsByBin histogram;
            // create a histogram of the sample
            Histogram(sampleStart, sampleEnd, histogram);

            // get the extents of the graphs to find the centre of the transmission.
            const auto maxIt = std::max_element(histogram.cbegin(), histogram.cend());


            // values are in "bins" so will need to be translated to time
            return distance(histogram.cbegin(), maxIt) * pulseWidth;
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
                cutoff.time = start->time + driftSampleTime * sampleIndex;
                // use the binary search to find the point in the data where the time is past our sample time limit
                if(!Filter::FindEdge<DetectionReport>(sampleStart, sampleEnd, cutoff, sampleEnd, [](const auto& left, const auto&right){
                   return left.time > right.time;
                }))
                {
                    LOGERROR("Failed to find the time slice, using the last element");
                }
                LOGDEBUG("Searching for peak in " + to_string(distance(sampleStart, sampleEnd)) + " samples");

                auto findStart = sampleStart;
                auto findEnd = sampleEnd;
                peakFutures.push_back(async(launch::async, &Gating::FindPeak, this, findStart, findEnd));

                // set the start of the next sample
                sampleStart = sampleEnd;
                sampleEnd = end;
                sampleIndex++;
            } while(sampleStart != end);


            PicoSecondOffset result{0};
            // average the seperation between the peaks
            // TODO: weighted average?
            if(peakFutures.size() > 1)
            {
                // get the value for the first peak
                PicoSeconds previousPeak {peakFutures.begin()->get()};
                // go through each peak and find the offset
                for(auto it = peakFutures.begin() + 1; it != peakFutures.end(); it++)
                {
                    auto thisPeak = it->get();
                    // average the value
                    PicoSecondOffset drift = previousPeak - thisPeak;
                    if(drift.count() < 0)
                    {
                        drift *= -1;
                    }
                    result = (result + drift) / 2;
                }
                //result = result / peakFutures.size();
                LOGDEBUG("calculated drift: " + to_string(result.count()));
            } else {
                LOGERROR("No enough data to calculate drift");
            }

            return result;
        }

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   ValidSlots& validSlots,
                                   QubitList& results,
                                   bool calculateDrift)
        {
            if(calculateDrift)
            {
                drift = CalculateDrift(start, end);
                LOGDEBUG("Drift = " + std::to_string(drift.count()));
            }
            CountsByBin counts;
            ResultsByBinBySlot resultsBySlot;
            CountDetections(start, end, counts, resultsBySlot);
            GateResults(counts, resultsBySlot, validSlots, results);

        } // ScoreOffsets

    } // namespace align
} // namespace cqp