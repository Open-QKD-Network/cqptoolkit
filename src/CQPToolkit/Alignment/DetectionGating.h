/*!
* @file
* @brief DetectionGating
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <vector>
#include <unordered_map>
#include "Algorithms/Datatypes/DetectionReport.h"
#include <future>
#include "Algorithms/Logging/Logger.h"
#include <unordered_set>
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Random/IRandom.h"

namespace cqp
{
    namespace align
    {

        /**
         * @brief The DetectionGating class
         * Calculates the most probable offset from the start of a slot for the moment that a photon arrives at the detector
         * This removes the need for a high resolution timing signal.
         * @details
         * @verbatim
         <- Slot Width (10ms) ->

          |--------_--------------
          |       |@|_           ^
          |      _|@|@|          | Acceptance Ratio
          |     |@|@|@|          | (0.5)
          |     |@|@|@|          v
          |-----|@|@|@|-----------
          |  _  |@|@|@|  _
          |_|#| |@|@|@|_|#|
          |#|#|_|@|@|@|#|#|_
          |#|#|#|@|@|@|#|#|#|
          '---------------------
          ^-^
          Pulse width (1ms)
         @endverbatim
         */
        class CQPTOOLKIT_EXPORT DetectionGating
        {
        public:
            /// How far away from the origin to check for a spike in the histogram
            constexpr static const uint64_t DefaultoffsetTestRange = 100;
            /// what is the minimum histogram count that will be accepted as a detection - allow for spread/drift
            constexpr static const double DefaultAcceptanceRatio = 0.1;

            /**
             * @brief DetectionGating
             * Default constructor
             * @param randGen The source of randomness
             */
            explicit DetectionGating(IRandom& randGen);

            /**
             * @brief SetSystemParameters
             * @details
             * frameWidth / slotWidth = number of slots
             * @param frameWidth Duration for a block of transmissions
             * @param slotWidth Duration for a single transmission window
             * @param pulseWidth Duration for a single photon
             * @param slotOffsetTestRange How many slots to offset to find a match between detections and markers
             * @param acceptanceRatio What percentage of the maximum to still consider part of the valid results
             */
            bool SetSystemParameters(PicoSeconds frameWidth, PicoSeconds slotWidth, PicoSeconds pulseWidth,
                            uint64_t slotOffsetTestRange = DefaultoffsetTestRange,
                            double acceptanceRatio = DefaultAcceptanceRatio);

            /**
             * @brief ResetDrift
             * @details thread safe call
             * @param newDrift
             */
            void ResetDrift(PicoSecondOffset newDrift)
            {
                calculatedDrift = newDrift;
            }

            /**
             * @brief SetNumberThreads
             * Sets the number of threads to spread the processing over.
             * @details thread safe call
             * @param threads If this is 0, the number of hardware threads in the system will be used.
             */
            void SetNumberThreads(uint threads);

            /**
             * @brief SetStartOffsetRange
             * Change the starting offset for detections.
             * @param offsetTestRange
             */
            void SetStartOffsetRange(uint64_t offsetTestRange)
            {
                slotOffsetTestRange = offsetTestRange;
            }

            /**
             * @brief BuildHistogram
             * @details Blocking call. This call will block if this class is currently processing data.
             * @startuml
             *  boundary "IAlignment" as ia
             *  participant "DetectionGating" as dg
             *  participant "Tread Pool" as tp
             *  participant "Summation" as sum
             *  control "Worker" as w
             *
             *  [-> dg : BuildHistogram
             *  activate dg
             *      loop for each thread
             *          dg -\ w : HistogramWorker(threadDataRange)
             *      activate w
             *      end loop
             *
             *  par
             *      dg -> ia : GetAlignmentMarks
             *  else
             *
             *      note over w
             *          Process the raw data
             *      end note
             *      w -> sum : notify_all()
             *  end par
             *
             *  note over dg
             *      Wait for threads to finish the first stage
             *  end note
             *  dg -> sum : wait()
             *  dg -> dg : CalculateDrift()
             *  note over dg
             *      Start the next stage
             *  end note
             *  dg -> sum : notify_all()
             *
             *  note over w
             *      Wait for target bin
             *  end note
             *  w -> sum : wait()
             *
             *  note over w
             *      Find high score
             *  end note
             *
             *  note over dg
             *      Wait for threads to finish scoring
             *  end note
             *
             *  loop for each thread
             *      dg -> w : join()
             *
             *      deactivate w
             *      destroy w
             *
             *      note over dg
             *          collect the highest score slot
             *      end note
             *  end loop
             *
             *  note over dg
             *      drop any unusable data
             *      and build a set of results
             *  end note
             *  dg -> ia : SendValidDetections()
             *  [<- dg : return valid results
             *  deactivate dg
             *  @enduml
             * @param source The raw data to process
             * @param frameId The id for this raw data
             * @param channel The peer to send valid detections to optimise the detections.
             * @return The final processed data.
             */
            std::unique_ptr<QubitList> BuildHistogram(
                    const DetectionReportList& source, SequenceNumber frameId, std::shared_ptr<grpc::Channel> channel);

        protected: // types

            /// The histogram storage type
            using CountsByBin = std::vector<uint64_t>;

            /// Identifier type for slots
            using SlotID = uint64_t;
            /// Identifier type bins
            using BinID = uint64_t;

            /// A list of slot ids
            using DetectedSlots = std::set<SlotID>;

            /// Upper and lower bounds for detections
            using DetectionBounds = std::pair<DetectionReportList::const_iterator, DetectionReportList::const_iterator>;

            /// Assumptions:
            /// the number of detections per slot per bin are sparse
            /// as the dataset is small, the number of bins with detections is also sparse
            /// this needs to be ordered so that the list can be collapsed, dripping the slots we missed
            using ValuesBySlot = std::map<SlotID, std::vector<const Qubit*>>;

            /// A list of results
            using ResultsByBin = std::unordered_map<BinID, ValuesBySlot>;

            /**
             * @brief The OffsetHighscore struct
             * Stores the high score a slot offset
             */
            struct OffsetHighscore
            {
                /// The offset from origin for this score
                uint64_t slotIdOffset = 0;
                /// The number of detections which match for this offset
                int score = 0;
            };

            /**
             * @brief The ThreadStaging struct
             * Details for the processing threads
             */
            struct ThreadStaging
            {
                /// The number of threads still running
                std::atomic<uint> threadsActive {0};
                /// Has a target been found
                bool targetBinFound = false;
                /// Have all results been collected
                bool resultsCollected = false;
                /// protect access to collecting results
                std::mutex summationMutex;
                /// to watch for a change to the results
                std::condition_variable summationCv;
            };

            /**
             * @brief The ThreadDetails struct
             * The tread settings and their scores
             */
            struct ThreadDetails
            {
                /// Thread managing a data block
                std::thread theThread;
                /// The highest scoring offset
                OffsetHighscore highScoreOffset;
            };

        protected: // system parameters

            /// The number of slots this frame is split into
            uint64_t numSlots         {100};
            /// The duration for each slot
            PicoSeconds slotWidth   {10000};
            /// The duration for a single photon (for filtering processes)
            PicoSeconds pulseWidth    {100};
            /// The number of possible positions within a slot for a detection
            uint64_t numBins {100};

        protected: // members

            /// source of randomness
            IRandom& randomGenerator;

            /// Settings for the threads
            ThreadStaging threadStaging;
            /// The number of concurrent processes to run
            std::atomic<uint> maxThreads {1};
            /// protection for processing runs
            std::mutex processingMutex;

            /// storage for counts across all threads
            CountsByBin globalCounts;
            /// storage for result data across all threads
            ValuesBySlot allResults;

            BinID minBinId = 0;
            BinID maxBinId = 0;
            /// drift stored as seconds/second as a fraction
            PicoSecondOffset calculatedDrift {0};
            double acceptanceRatio = DefaultAcceptanceRatio;
            uint64_t slotOffsetTestRange {DefaultoffsetTestRange};

            std::vector<ThreadDetails> threadPool;

        protected: // methods
            /**
             * @brief HistogramWorker
             * @param dataBounds
             * @param myOffsetRange
             * @param markers
             * @param offsetHighscore
             */
            void HistogramWorker(
                    const DetectionBounds& dataBounds, std::pair<uint64_t, uint64_t> myOffsetRange,
                    const remote::QubitByIndex &markers, OffsetHighscore &offsetHighscore);

            /**
             * @brief GetMarkers
             * @param channel
             * @param frameId
             * @param results
             * @return ok on success
             */
            grpc::Status GetMarkers(
                    std::shared_ptr<grpc::Channel> channel, SequenceNumber frameId, remote::QubitByIndex& results);
            /**
             * @brief SendValidDetections
             * @param channel
             * @param frame
             * @param results
             * @param offset
             * @return ok on success
             */
            static grpc::Status SendValidDetections(
                    std::shared_ptr<grpc::Channel> channel, SequenceNumber frame, const DetectedSlots& results,
                    uint64_t offset);
            /**
             * @brief ScoreOffsets
             * @param myOffsetRange
             * @param markers
             * @param allResults
             * @return
             */
            static OffsetHighscore ScoreOffsets(const std::pair<uint64_t, uint64_t>& myOffsetRange,
                                       const remote::QubitByIndex& markers, const ValuesBySlot &allResults);

            /**
             * @brief CountDetections
             * @param dataBounds
             * @param myResults
             */
            void CountDetections(const DetectionGating::DetectionBounds& dataBounds, ResultsByBin &myResults);

            /**
             * @brief CalculateDrift
             */
            void CalculateDrift();
        };

    } // namespace align
} // namespace cqp


