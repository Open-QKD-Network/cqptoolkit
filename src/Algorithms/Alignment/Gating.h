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
#include "Algorithms/Alignment/AlignmentTypes.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Gating class extracts valid qubits from noise.
         */
        class ALGORITHMS_EXPORT Gating
        {
        public: // types
            /// what is the minimum histogram count that will be accepted as a detection - allow for spread/drift
            static constexpr double DefaultAcceptanceRatio = 0.2;

            /// Assumptions:
            /// the number of detections per slot per bin are sparse
            /// as the data set is small, the number of bins with detections is also sparse
            /// this needs to be ordered so that the list can be collapsed, dropping the slots we missed
            using ValuesBySlot = std::map<SlotID, std::vector<Qubit>>;

            /// A list of results with each bin is a list of slots for the data
            using ResultsByBinBySlot = std::vector</*BinID,*/ ValuesBySlot>;

            /// The histogram storage type
            using CountsByBin = std::vector<BinID>;
            /// A list of slot ids
            using ValidSlots = std::vector<SlotID>;

        public: // methods

            /**
             * @brief Gating Constructor
             * @param rng The random number generator used for choosing qubits for duplicate slots
             * @param slotWidth Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
             * @param txJitter The detection window for a qubit.
             * @param acceptanceRatio The percentage (0 to 1) of counts required for the slice on the histogram to be included in the counts.
             * A higher ratio mean less of the peak detections is accepted and less noise.
             */
            Gating(std::shared_ptr<IRandom> rng,
                   const PicoSeconds& slotWidth,
                   const PicoSeconds& txJitter,
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
             */
            void ExtractQubits(const DetectionReportList::const_iterator& start,
                               const DetectionReportList::const_iterator& end,
                               ValidSlots& validSlots,
                               QubitList& results);

            /**
             * @brief SetDrift
             * Change the drift value used for ExtractQubits when calculateDrift is set to false
             * @tparam T The ratio type which scales the time
             * @param newDrift The new drift value
             */
            template<typename T>
            void SetDrift(std::chrono::duration<int64_t, T> newDrift)
            {
                drift = std::chrono::duration_cast<SecondsDouble>(newDrift).count();
            }

            /**
             * @brief SetDrift
             * Change the drift value used for ExtractQubits when calculateDrift is set to false
             * @param newDrift The new drift value in seconds per second
             */
            void SetDrift(double newDrift)
            {
                drift = newDrift;
            }

            void SetChannelCorrections(ChannelOffsets newChannelCorrections)
            {
                channelCorrections = newChannelCorrections;
            }

            /**
             * @brief CountDetections
             * Build a historgram of the data while applying drift.
             * Also returns the qubits seperated by slot
             * @param[in] frameStart The estimated frame start time which will be used to offset all time values
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

            /**
             * @brief FilterDetections
             * Remove elements from qubits which do not have an index in validSlots
             * validSlots: { 0, 2, 3 }
             * Qubits:     { 8, 9, 10, 11 }
             * Result:     { 8, 10, 11 }
             * @param[in] validSlotsBegin The start of a list of indexes to filter the qubits
             * @param[in] validSlotsEnd The end of a list of indexes to filter the qubits
             * @param[in,out] qubits A list of qubits which will be reduced to the size of validSlots
             * @param[in] offset Shift the slot id
             * @return true on success
             */
            template<typename Iter>
            static bool FilterDetections(Iter validSlotsBegin, Iter validSlotsEnd, QubitList& qubits, int_fast32_t offset = 0)
            {
                using namespace std;
                bool result = false;

                const size_t validSlotsSize = distance(validSlotsBegin, validSlotsEnd);

                if(validSlotsSize <= qubits.size())
                {
                    size_t index = 0;
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

        protected: // members
            /// radom number generator for choosing from multiple qubits
            std::shared_ptr<IRandom> rng;
            /// Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
            const PicoSeconds slotWidth;
            /// The detection window for a qubit.
            const PicoSeconds txJitter;
            /// slotWidth / pulseWidth
            const BinID numBins;
            /// The percentage (0 to 1) of counts required for the slice on the histogram to be included in the counts.
            /// A higher ratio mean less of the peak detections is accepted and less noise.
            double acceptanceRatio;
            /// clock drift between tx and rx
            double drift {0.0};
            /// Amount of time to offset each channel to bring them perfectly overlapped.
            ChannelOffsets channelCorrections {};
        };

    } // namespace align
} // namespace cqp


