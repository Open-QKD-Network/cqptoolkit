/*!
* @file
* @brief Gating
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 01/02/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Gating.h"
#include <cmath>
#include <future>
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Alignment/Filter.h"
#include "Algorithms/Util/Maths.h"

namespace cqp
{
    namespace align
    {

        Gating::Gating(std::shared_ptr<IRandom> rng,
                       const PicoSeconds& slotWidth, const PicoSeconds& txJitter,
                       double acceptanceRatio):
            rng(move(rng)),
            slotWidth{slotWidth}, txJitter{txJitter},
            numBins{static_cast<BinID>(slotWidth / txJitter)},
            acceptanceRatio{acceptanceRatio}
        {

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
                const PicoSecondOffset offset { static_cast<PicoSecondOffset::rep>(round(drift * detection->time.count())) };

                // offset the time without the original value being converted to a float
                PicoSeconds  adjustedTime = detection->time - frameStart;
                // if the offset is positive, don't wrap past 0
                if(offset < AttoSeconds(0) || adjustedTime > offset)
                {
                    adjustedTime += channelCorrections[detection->value];
                    adjustedTime -= offset;
                }
                // C++ truncates on integer division
                const SlotID slot = DivNearest(adjustedTime.count(), slotWidth.count());

                const auto fromSlotStart = adjustedTime % slotWidth;
                //const BinID bin = div_to_nearest(fromSlotStart.count(), pulseWidth.count()) % numBins;
                const BinID bin = (fromSlotStart.count() / txJitter.count()) % numBins;
                // store the value as a bin for later access
                slotResults[bin][slot].push_back(detection->value);
                counts[bin]++;
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

            {
                auto findLower = std::async(std::launch::async, [&]()
                {
                    auto nextLower = lower;
                    while(counts[nextLower] > cutoff && nextLower != (peakIndex + 1) % numBins)
                    {
                        lower = nextLower;
                        nextLower = (numBins + nextLower - 1) % numBins;
                    }
                });

                while(counts[upper] > cutoff && upper != (peakIndex - 1) % numBins)
                {
                    upper = (upper + 1) % numBins;
                }

                findLower.wait();
            }

            LOGDEBUG(" lower=" + to_string(lower) + "Peak=" + to_string(peakIndex) + " upper=" + to_string(upper));
            map<SlotID, QubitList> qubitsBySlot;
            // walk through each bin, wrapping around to the start if the upper bin < lower bin

            uint64_t binCount = 0;

            for(auto binId = lower; binId != upper; binId = (binId + 1) % numBins)
            {
                SlotID slotOffset = 0;
                // If the bin ID is less than the lower limit we're left of bin 0
                if(upper < lower && binId < upper)
                {
                    // the peak wraps around, adjust the slot id for bins to the left
                    slotOffset = 1;
                }
                binCount++;
                for(const auto& slot : slotResults[binId])
                {
                    const auto mySlot = slot.first + slotOffset;
                    // add the qubits to the list for this slot, one will be chosen at random later
                    qubitsBySlot[mySlot].insert(qubitsBySlot[mySlot].end(), slot.second.cbegin(), slot.second.cend());

                } // for each slot
            } // for each bin

            if(peakWidth)
            {
                *peakWidth = (binCount / static_cast<double>(numBins));
            }

            uint_fast16_t multiSlots = 0;

            // as the list is ordered, the qubits will come out in the correct order
            // just append them to the result list
            for(const auto& list : qubitsBySlot)
            {
                if(!list.second.empty())
                {
                    // record that we have a value for this slot
                    validSlots.push_back(list.first);
                    if(list.second.size() == 1)
                    {
                        results.push_back(list.second[0]);
                    }
                    else
                    {
                        multiSlots++;
                        //LOGDEBUG("Multiple qubits for slot");
                        // pick a qubit at random
                        const auto index = rng->RandULong() % list.second.size();
                        results.push_back(list.second[index]);
                    }

                }
            } // for each qubitsBySlot

            LOGDEBUG("Number of multi-qubit slots: " + to_string(multiSlots));

            // results now contains a contiguous list of qubits
            // validSlots tells the caller which slots were used to create that list.
        } // CountDetections

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   ValidSlots& validSlots,
                                   QubitList& results)
        {
            using namespace std::chrono;

            LOGDEBUG("Drift = " + std::to_string(drift) + "s/s");
            CountsByBin counts;
            ResultsByBinBySlot resultsBySlot;
            CountDetections(start->time, start, end, counts, resultsBySlot);

            double peakWidth = 0.0;
            GateResults(counts, resultsBySlot, validSlots, results, &peakWidth);

            stats.PeakWidth.Update(peakWidth);
            stats.Drift.Update(drift);

            LOGDEBUG("Peak width: " + std::to_string(peakWidth * 100.0) + "%");


        }

    } // namespace align
} // namespace cqp
