#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include <map>
#include <vector>
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/algorithms_export.h"
#include <set>
#include "Algorithms/Random/IRandom.h"
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp {
    namespace align {

        class ALGORITHMS_EXPORT Gating
        {
        public: // types
            /// The length of a frame in number of slot widths
            static constexpr uint64_t DefaultSlotsPerFrame { 40000000 };
            /// The detection window for a qubit.
            static constexpr PicoSeconds DefaultPulseWidth {PicoSeconds(500)};
            /// Picoseconds of time in which one qubit can be detected.
            static constexpr PicoSeconds DefaultSlotWidth  {std::chrono::nanoseconds(100)};
            /// what is the minimum histogram count that will be accepted as a detection - allow for spread/drift
            static constexpr double DefaultAcceptanceRatio = 0.2;

            static constexpr PicoSeconds DefaultDriftResolution { PicoSeconds(100) };
            static constexpr PicoSeconds DefaultMaxDrift { std::chrono::nanoseconds(100) };

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
            using ResultsByBinBySlot = std::vector</*BinID,*/ ValuesBySlot>;

            /// The histogram storage type
            using CountsByBin = std::vector<uint64_t>;
            /// A list of slot ids
            using ValidSlots = std::vector<SlotID>;

        public: // methods

            /**
             * @brief Gating Constructor
             * @param rng The random number generator used for choosing qubits for duplicate slots
             * @param slotWidth Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
             * @param pulseWidth The detection window for a qubit.
             * @param samplesPerFrame The number of slots used to calculate drift
             * @param acceptanceRatio The percentage (0 to 1) of counts required for the slice on the histogram to be included in the counts.
             * A higher ratio mean less of the peak detections is accepted and less noise.
             */
            Gating(std::shared_ptr<IRandom> rng,
                   uint64_t slotsPerFrame = DefaultSlotsPerFrame,
                   const PicoSeconds& slotWidth = DefaultSlotWidth,
                   const PicoSeconds& pulseWidth = DefaultPulseWidth,
                   const PicoSeconds& driftResolution = DefaultDriftResolution,
                   const PicoSeconds& maxDrift = DefaultMaxDrift,
                   double acceptanceRatio = DefaultAcceptanceRatio);

            /**
             * @brief ExtractQubits
             * Perform detection counting, drift calculation, scoring, etc to produce a list of qubits from raw detections.
             * @details
             * @startuml
             *  [-> Gating : ExtractQubits
             *  activate Gating
             *  alt if calculateDrift
             *      Gating -> Gating : CalculateDrift
             *  end alt
             *  Gating -> Gating : CountDetections
             *  Gating -> Gating : GateResults
             *  @enduml
             * @param[in] start The start of the raw data
             * @param[in] end The end of the raw data
             * @param[out] validSlots A list of slot id which were successfully extracted
             * @param[out] results The qubit values for each slot id in validSlots
             * @param[in] calculateDrift When true, a new drift value will be calculated before performing gating,
             * otherwise the current value will be used.
             */
            void ExtractQubits(const DetectionReportList::const_iterator& start,
                               const DetectionReportList::const_iterator& end,
                               ValidSlots& validSlots,
                               QubitList& results,
                               bool calculateDrift = true);

            /**
             * @brief SetDrift
             * Change the drift value used for ExtractQubits when calculateDrift is set to false
             * @param newDrift The new drift value
             */
            void SetDrift(SecondsDouble newDrift)
            {
                drift = newDrift;
            }

            /**
             * @brief Histogram
             * Create a histogram of the data by counting the occorences
             * @param[in] start The start of the data
             * @param[in] end The end of the data
             * @param[in] numBins The number of columns in the histogram
             * @param[in] windowWidth The width in time of the histogram window
             * @param[out] counts The result of counting the times based on the pulse width setting
             */
            static void Histogram(const DetectionReportList::const_iterator& start,
                           const DetectionReportList::const_iterator& end,
                           uint64_t numBins,
                           PicoSeconds windowWidth,
                           Gating::CountsByBin& counts);

            /**
             * @brief CalculateDrift
             * successivly sample the data and measure the disance between peaks to detect clock drift
             * @param start Start of data to sample
             * @param end End of data to sample
             * @return Picoseconds drift
             */
            SecondsDouble CalculateDrift(const DetectionReportList::const_iterator& start,
                                            const DetectionReportList::const_iterator& end) const;

            /**
             * @brief CountDetections
             * Build a historgram of the data while applying drift.
             * Also returns the qubits seperated by slot
             * @param[in] start Start of data to count
             * @param[in] end End of data to count
             * @param[out] counts The histogram of the data
             * @param[out] slotResults The qubits arranged by bin and slot
             */
            void CountDetections(const PicoSeconds& frameStart,
                                 const DetectionReportList::const_iterator& start,
                                 const DetectionReportList::const_iterator& end,
                                 CountsByBin& counts,
                                 ResultsByBinBySlot& slotResults) const;

            /**
             * @brief GateResults
             * Filter out detections which dont pass the acceptance value
             * @param[in] counts Histogram of detections
             * @param[in] slotResults qubit values to be filtered
             * @param[out] validSlots The slots which contain valid detections
             * Gaurenteed to be in assending order
             * @param[out] results The usable qubit values
             * @param[out] peakWidth Optional. if not null, will be filled with the percentage (0 - 1) of the histogram which was accepted.
             * The larger the width the more noise is present
             */
            void GateResults(const CountsByBin& counts,
                             const ResultsByBinBySlot& slotResults,
                             ValidSlots& validSlots,
                             QubitList& results,
                             double* peakWidth = nullptr) const;

            using SparseQubitList = std::map<size_t, Qubit>;

            static double EstimateMatch(SparseQubitList truthValues, QubitList imperfectValues)
            {
                size_t valid = 0;
                size_t numResults = 0;
                for(auto truthIt = truthValues.cbegin(); truthIt != truthValues.cend(); truthIt++)
                {
                    if(truthIt->first < imperfectValues.size() && QubitHelper::Base(truthIt->second) == QubitHelper::Base(imperfectValues[truthIt->first]))
                    {
                        numResults++;
                        if(truthIt->second == imperfectValues[truthIt->first])
                        {
                            valid++;
                        }
                    }
                }

                return static_cast<double>(valid) / numResults;
            }

            /**
             * @brief FilterDetections
             * Remove elements from qubits which do not have an index in validSlots
             * validSlots: { 0, 2, 3 }
             * Qubits:     { 8, 9, 10, 11 }
             * Result:     { 8, 10, 11 }
             * @param[in] validSlotsBegin The start of a list of indexes to filter the qubits
             * @param[in] validSlotsEnd The end of a list of indexes to filter the qubits
             * @param[in,out] qubits A list of qubits which will be reduced to the size of validSlots
             * @return true on success
             */
            template<typename Iter>
            static bool FilterDetections(Iter validSlotsBegin, Iter validSlotsEnd, QubitList& qubits, ssize_t offset = 0)
            {
                using namespace std;
                bool result = false;

                const size_t validSlotsSize = distance(validSlotsBegin, validSlotsEnd);

                if(validSlotsSize <= qubits.size())
                {
                    uint64_t index = 0;
                    for(auto validSlot = validSlotsBegin; validSlot != validSlotsEnd; validSlot++)
                    {
                        const auto adjustedSlot = *validSlot + offset;
                        if(adjustedSlot >= 0 && adjustedSlot < qubits.size())
                        {
                            qubits[index] = qubits[adjustedSlot];
                            index++;
                        } else {
                            LOGERROR("Invalid SlotID");
                        }
                    }

                    // through away the bits on the end
                    qubits.resize(validSlotsSize);
                    result = true;
                }

                return result;
            }


            /**
             * @brief The Statistics struct
             * The statistics reported by Gating
             */
            struct Stats : stats::StatCollection
            {
                /// The group to contain these stats
                const char* parent = "Gating";
                /// Peak width for signal quality
                stats::Stat<double> PeakWidth {{parent, "PeakWidth"}, stats::Units::Percentage, "A measurement of the accuracy of detections and drift"};

                /// The time took to transmit the qubits
                stats::Stat<double> Drift {{parent, "Drift"}, stats::Units::PicoSecondsPerSecond, "The clock drift between the transmitter and detector"};

                /// @copydoc stats::StatCollection::Add
                void Add(stats::IAllStatsCallback* statsCb) override
                {
                    PeakWidth.Add(statsCb);
                    Drift.Add(statsCb);
                }

                /// @copydoc stats::StatCollection::Remove
                void Remove(stats::IAllStatsCallback* statsCb) override
                {
                    PeakWidth.Remove(statsCb);
                    Drift.Remove(statsCb);
                }

                /// @copydoc stats::StatCollection::AllStats
                std::vector<stats::StatBase*> AllStats() override
                {
                    return
                    {
                        &PeakWidth, &Drift
                    };
                }
            } stats;

        protected: // methods

            /**
             * @brief FindPeak
             * Createa histogram of the data and find the highest count
             * @param sampleStart start of data to read
             * @param sampleEnd end of data to read
             * @return The time in picoseconds from the start of the data
             */
            PicoSeconds FindPeak(DetectionReportList::const_iterator sampleStart,
                                  DetectionReportList::const_iterator sampleEnd) const;

        protected: // members
            /// radom number generator for choosing from multiple qubits
            std::shared_ptr<IRandom> rng;
            /// The length of a frame in number of slotWidths
            const uint64_t slotsPerFrame;
            /// Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
            const PicoSeconds slotWidth;
            /// The detection window for a qubit.
            const PicoSeconds pulseWidth;
            PicoSeconds driftResolution;
            PicoSeconds maxDrift;
            /// slotWidth / pulseWidth
            const uint64_t numBins;
            uint64_t driftBins;
            PicoSeconds driftSampleTime;
            /// The percentage (0 to 1) of counts required for the slice on the histogram to be included in the counts.
            /// A higher ratio mean less of the peak detections is accepted and less noise.
            double acceptanceRatio;
            /// clock drift between tx and rx
            SecondsDouble drift {0};
        };

    } // namespace align
} // namespace cqp


