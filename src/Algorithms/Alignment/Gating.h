#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include <map>
#include <vector>
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/algorithms_export.h"
#include <set>
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
                   uint64_t samplesPerFrame = 40,
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

            using ValidSlots = std::vector<SlotID>;

            /**
             * @brief GateResults
             * @param counts
             * @param slotResults
             * @param validSlots
             * Gaurenteed to be in assending order
             * @param results
             */
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

            /**
             * @brief FilterDetections
             * Remove elements from qubits which do not have an index in validSlots
             * validSlots: { 1, 3, 4 }
             * Qubits:     { 8, 9, 10, 11 }
             * Result:     { 8, 10, 11 }
             * @param validSlots A list of indexes to filter the qubits
             * @param qubits A list of qubits which will be reduced to the size of validSlots
             * @return true on success
             */
            template<typename Iter>
            static bool FilterDetections(Iter validSlotsBegin, Iter validSlotsEnd, QubitList& qubits)
            {
                using namespace std;
                bool result = false;
                const size_t validSlotsSize = distance(validSlotsBegin, validSlotsEnd);

                if(validSlotsSize <= qubits.size())
                {
                    uint64_t index = 0;
                    for(auto validSlot = validSlotsBegin; validSlot != validSlotsEnd; validSlot++)
                    {
                        qubits[index] = qubits[*validSlot];
                        index++;
                    }
                    // through away the bits on the end
                    qubits.resize(validSlotsSize);
                    result = true;
                }

                return result;
            }

        protected:
            std::shared_ptr<IRandom> rng;
            const PicoSeconds slotWidth;
            const PicoSeconds pulseWidth;
            const uint64_t samplesPerFrame;
            const uint64_t numBins;
            double acceptanceRatio;
            PicoSecondOffset drift {0};
        };

    } // namespace align
} // namespace cqp


