#include "Gating.h"
#include <cmath>
#include "Algorithms/Logging/Logger.h"

namespace cqp {
    namespace align {

        // This deifines the storage unit for the constexpr in the header
        // this looks weird because the constant is defined in the header
        constexpr PicoSeconds Gating::DefaultPulseWidth;
        constexpr PicoSeconds Gating::DefaultSlotWidth;

        Gating::Gating(const PicoSeconds& slotWidth, const PicoSeconds& pulseWidth, double acceptanceRatio):
            slotWidth{slotWidth}, pulseWidth{pulseWidth}, numBins{slotWidth / pulseWidth},
            acceptanceRatio{acceptanceRatio}
        {

        }

        void Gating::CountDetections(const DetectionReportList::const_iterator& start,
                                    const DetectionReportList::const_iterator& end,
                                    Gating::CountsByBin& counts,
                                    ResultsByBinBySlot& slotResults,
                                     const PicoSecondOffset& drift) const
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
                // TODO: get rid of the literal!
                const auto test = detection->time.count();
                const PicoSecondOffset offset(static_cast<int64_t>(
                                                  ceil((static_cast<double>(drift.count()) * static_cast<double>(test)) / 1000000000.0)
                                                  ));
                // offset the time without the original value being converted to a float
                const PicoSeconds adjustedTime = detection->time + offset;
                // C++ truncates on integer division
                const SlotID slot = adjustedTime / slotWidth;
                const BinID bin = (adjustedTime % slotWidth) / pulseWidth;
                // store the value as a bin for later access
                slotResults[bin][slot].push_back(detection->value);
                // the value is atomic so we can safely update it
                counts[bin]++;
            }

        } // CountDetections

        PicoSecondOffset Gating::CalculateDrift(const Gating::CountsByBin& counts,
                                    BinID& targetBinId, BinID& minBinId, BinID& maxBinId) const
        {
            using namespace std;
            targetBinId = 0;

            for(uint64_t index = 0; index < counts.size(); index++)
            {
                if(counts[index] > counts[targetBinId])
                {
                    targetBinId = index;
                }
            } // for counts

            minBinId = targetBinId;
            maxBinId = targetBinId;
            const uint64_t minCount = max(1ul, static_cast<uint64_t>(counts[targetBinId] * acceptanceRatio));

            int16_t driftOffset = 0;
            for(uint64_t index = 1; index < numBins; index++)
            {
                const auto indexToCheck = (targetBinId + index) % numBins;
                // using wrapping, look right of the peek
                if(counts[indexToCheck] >= minCount)
                {
                    // nudge the drift to the right
                    driftOffset++;
                    maxBinId = indexToCheck;
                } else {
                    break; // for
                }
            }

            for(uint64_t index = 1; index < numBins; index++)
            {
                const BinID indexToCheck = (numBins + targetBinId - index) % numBins;
                // find the first one which is too small
                if(counts[indexToCheck] >= minCount)
                {
                    // nudge the drift to the left
                    driftOffset--;
                    minBinId = indexToCheck;
                } else {
                    break; // for
                }
            }

            if(minBinId == targetBinId + 1 && maxBinId == targetBinId - 1)
            {
                LOGERROR("All bins within drift tolerance. Noise level too high.");
            }

            return pulseWidth * driftOffset;
        } // CalculateDrift

        void Gating::ExtractQubits(const DetectionReportList::const_iterator& start,
                                   const DetectionReportList::const_iterator& end,
                                   const QubitsBySlot& markers,
                                   QubitList& results)
        {
            CountsByBin counts;
            ResultsByBinBySlot indexedResults;
            CountDetections(start, end, counts, indexedResults, drift);

            BinID targetBinId;
            BinID minBinId;
            BinID maxBinId;
            // move the drift by half to make a small correction
            drift += CalculateDrift(counts, targetBinId, minBinId, maxBinId) / 2;

        } // ScoreOffsets

    } // namespace align
} // namespace cqp
