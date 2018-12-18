#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include <map>
#include <vector>
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/algorithms_export.h"
#include <unordered_set>
#include "Algorithms/Random/IRandom.h"

namespace cqp {
    namespace align {

        class ALGORITHMS_EXPORT Gating
        {
        public:
            static constexpr PicoSeconds DefaultPulseWidth {PicoSeconds(500)};
            static constexpr PicoSeconds DefaultSlotWidth  {std::chrono::nanoseconds(100)};
            /// what is the minimum histogram count that will be accepted as a detection - allow for spread/drift
            static constexpr double DefaultAcceptanceRatio = 0.5;

            Gating(std::shared_ptr<IRandom> rng, const PicoSeconds& slotWidth = DefaultSlotWidth,
                  const PicoSeconds& pulseWidth = DefaultPulseWidth,
                   uint64_t slotsPerDriftSample = 1000000,
                   double acceptanceRatio = DefaultAcceptanceRatio);

            /// Identifier type for slots
            using SlotID = uint64_t;

            /// Assumptions:
            /// the number of detections per slot per bin are sparse
            /// as the data set is small, the number of bins with detections is also sparse
            /// this needs to be ordered so that the list can be collapsed, dropping the slots we missed
            using ValuesBySlot = std::map<SlotID, std::vector<Qubit>>;

            /// Identifier type bins
            using BinID = uint64_t;
            /// A list of results with each bin is a list of slots for the data
            using ResultsByBinBySlot = std::unordered_map<BinID, ValuesBySlot>;

            /// The histogram storage type
            using CountsByBin = std::vector<uint64_t>;

            static DetectionReportList::const_iterator FindPointInTime(const DetectionReportList::const_iterator& start,
                                                                const DetectionReportList::const_iterator& end,
                                                                const PicoSeconds& point);

            void Histogram(const DetectionReportList::const_iterator& start,
                           const DetectionReportList::const_iterator& end,
                           Gating::CountsByBin& counts) const;

            PicoSeconds FindPeak(DetectionReportList::const_iterator sampleStart,
                                  DetectionReportList::const_iterator sampleEnd) const;

            PicoSecondOffset CalculateDrift(const DetectionReportList::const_iterator& start,
                                            const DetectionReportList::const_iterator& end);

            void CountDetections(const DetectionReportList::const_iterator& start,
                                 const DetectionReportList::const_iterator& end,
                                 CountsByBin& counts,
                                 ResultsByBinBySlot& slotResults) const;

            using ValidSlots = std::unordered_set<SlotID>;

            void GateResults(const CountsByBin& counts,
                             const ResultsByBinBySlot& slotResults,
                             ValidSlots& validSlots,
                             QubitList& results);

            /**
             * Perform detection counting, drift calculation, scoring, etc to produce a list of qubits from raw detections.
             */
            void ExtractQubits(const DetectionReportList::const_iterator& start,
                               const DetectionReportList::const_iterator& end,
                               ValidSlots& validSlots,
                               QubitList& results,
                               bool calculateDrift = true);
        protected:
            std::shared_ptr<IRandom> rng;
            const PicoSeconds slotWidth;
            const PicoSeconds pulseWidth;
            const uint64_t slotsPerDriftSample;
            const uint64_t numBins;
            double acceptanceRatio;
            PicoSecondOffset drift {0}; //{34794};
        };

    } // namespace align
} // namespace cqp


