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
#include "CQPToolkit/Datatypes/DetectionReport.h"
#include <future>
#include "CQPToolkit/Util/Logger.h"
#include <unordered_set>
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Interfaces/IRandom.h"

namespace cqp
{
    namespace align
    {

        class CQPTOOLKIT_EXPORT DetectionGating
        {
        public:
            // signed duration value to allow drift to go in both directions
            using PicoSecondOffset = std::chrono::duration<int64_t, PicoSeconds::period>;

            constexpr static const uint64_t DefaultoffsetTestRange = 100;
            constexpr static const double DefaultAcceptanceRatio = 0.1;

            explicit DetectionGating(IRandom& randGen);

            /**
             * @brief DetectionGating
             * @param randGen
             * @param frameWidth
             * @param slotWidth
             * @param pulseWidth
             * @param slotOffsetTestRange How many slots to offset to find a match between detections and markers
             * @param acceptanceRatio
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

            void SetStartOffsetRange(uint64_t offsetTestRange)
            {
                slotOffsetTestRange = offsetTestRange;
            }

            /**
             * @brief BuildHistogram
             * @details Blocking call. This call will block if this class is currently processing data.
             * @param source
             * @param results
             * @param channel
             */
            std::unique_ptr<QubitList> BuildHistogram(const DetectionReportList& source, SequenceNumber frameId, std::shared_ptr<grpc::Channel> channel);

        protected: // types

            using CountsByBin = std::vector<uint64_t>;

            using SlotID = uint64_t;
            using BinID = uint64_t;

            using DetectedSlots = std::set<SlotID>;

            using DetectionBounds = std::pair<DetectionReportList::const_iterator, DetectionReportList::const_iterator>;

            // assumtions:
            // the number of detections per slot per bin are sparce
            // as the dataset is small, the number of bins with detections is also sparse
            // this needs to be ordered so that the list can be collapsed, dripping the slots we missed
            using ValuesBySlot = std::map<SlotID, std::vector<const Qubit*>>;

            using ResultsByBin = std::unordered_map<BinID, ValuesBySlot>;

            struct OffsetHighscore
            {
                uint64_t slotIdOffset = 0;
                int score = 0;
            };

            struct ThreadStaging
            {
                std::atomic<uint> threadsActive {0};
                bool targetBinFound = false;
                bool resultsCollected = false;
                std::mutex summationMutex;
                std::condition_variable summationCv;
            };

            struct ThreadDetails
            {
                std::thread theThread;
                OffsetHighscore highScoreOffset;
            };

        protected: // system parameters

            uint64_t numSlots         {100};
            PicoSeconds slotWidth   {10000};
            PicoSeconds pulseWidth    {100};
            uint64_t numBins {100};

        protected: // members

            IRandom& randomGenerator;

            ThreadStaging threadStaging;
            std::atomic<uint> maxThreads {1};
            std::mutex processingMutex;

            CountsByBin globalCounts;
            ValuesBySlot allResults;

            BinID minBinId = 0;
            BinID maxBinId = 0;
            // drift stored as seconds/second as a fraction
            PicoSecondOffset calculatedDrift {0};
            double acceptanceRatio = DefaultAcceptanceRatio;
            uint64_t slotOffsetTestRange {DefaultoffsetTestRange};

            std::vector<ThreadDetails> threadPool;

        protected: // methods
            void HistogramWorker(const DetectionBounds& dataBounds, std::pair<uint64_t, uint64_t> myOffsetRange, const remote::QubitByIndex &markers, OffsetHighscore &offsetHighscore);

            grpc::Status GetMarkers(std::shared_ptr<grpc::Channel> channel, SequenceNumber frameId, remote::QubitByIndex& results);
            static grpc::Status SendValidDetections(std::shared_ptr<grpc::Channel> channel, SequenceNumber frame, const DetectedSlots& results, uint64_t offset);
            static OffsetHighscore ScoreOffsets(const std::pair<uint64_t, uint64_t>& myOffsetRange,
                                       const remote::QubitByIndex& markers, const ValuesBySlot &allResults);

            void CountDetections(const DetectionGating::DetectionBounds& dataBounds, ResultsByBin &myResults);
            void CalculateDrift();
        };

    } // namespace align
} // namespace cqp


