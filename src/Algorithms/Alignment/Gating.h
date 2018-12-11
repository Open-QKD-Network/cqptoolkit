#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include <map>
#include <vector>
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/algorithms_export.h"

namespace cqp {
    namespace align {

        class ALGORITHMS_EXPORT Gating
        {
        public:
            static constexpr PicoSeconds DefaultPulseWidth {100};
            static constexpr PicoSeconds DefaultSlotWidth  {10000};
            /// what is the minimum histogram count that will be accepted as a detection - allow for spread/drift
            static constexpr double DefaultAcceptanceRatio = 0.5;

            Gating(const PicoSeconds& slotWidth = DefaultSlotWidth,
                  const PicoSeconds& pulseWidth = DefaultPulseWidth,
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

            void CountDetections(const DetectionReportList::const_iterator& start,
                                 const DetectionReportList::const_iterator& end,
                                 CountsByBin& counts,
                                 ResultsByBinBySlot& slotResults,
                                 const PicoSecondOffset& drift = PicoSecondOffset(0)) const;

            /**
             * @brief CalculateDrift
             */
            PicoSecondOffset CalculateDrift(const Gating::CountsByBin& counts,
                                BinID& targetBinId, BinID& minBinId, BinID& maxBinId) const;

            using QubitsBySlot = std::map<SlotID, Qubit>;

            /**
             * Perform detection counting, drift calculation, scoring, etc to produce a list of qubits from raw detections.
             */
            void ExtractQubits(const DetectionReportList::const_iterator& start,
                               const DetectionReportList::const_iterator& end,
                               const QubitsBySlot& markers,
                               QubitList& results);
        protected:
            const PicoSeconds slotWidth;
            const PicoSeconds pulseWidth;
            const uint64_t numBins;
            double acceptanceRatio;
            PicoSecondOffset drift {0};
        };

    } // namespace align
} // namespace cqp


