/*!
* @file
* @brief Drift
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 9/11/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include "Algorithms/Alignment/AlignmentTypes.h"
#include "Algorithms/Util/ProcessingQueue.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Drift class calculates drift based on detections
         * The algorithm has been adapted from the work of Dr David Lowndes <David.Lowndes@bristol.ac.uk> in
         * @cite HandHeld
         */
        class ALGORITHMS_EXPORT Drift
        {
        public:
            /// seperation between samples to find clock peak
            static constexpr PicoSeconds DefaultDriftSampleTime { std::chrono::milliseconds(100) };

            /**
             * @brief Drift constructor
             * @param slotWidth time between transmissions
             * @param txJitter Transmitter clock jitter
             * @param driftSampleTime seperation between samples to find clock peak
             */
            Drift(const PicoSeconds& slotWidth, const PicoSeconds& txJitter,
                  const PicoSeconds& driftSampleTime = DefaultDriftSampleTime);

            /**
             * @brief CalculateDrift
             * successivly sample the data and measure the disance between peaks to detect clock drift
             * @param start Start of data to sample
             * @param end End of data to sample
             * @return Picoseconds drift
             */
            double Calculate(const DetectionReportList::const_iterator& start,
                            const DetectionReportList::const_iterator& end);

            /**
             * @brief ChannelFindPeak
             * Find the offset between the channels
             * @param sampleStart data to sample, start
             * @param sampleEnd data to sample, end
             * @return The relative offsets between the channels
             */
            ChannelOffsets ChannelFindPeak(DetectionReportList::const_iterator sampleStart,
                                  DetectionReportList::const_iterator sampleEnd) const;

        protected: // methods

            /// A list of histograms, one for each channel
            using ChannelHistograms = std::array<std::vector<uint64_t>, maxChannels>;

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
                           uint_fast16_t numBins,
                           PicoSeconds windowWidth,
                           std::vector<uint64_t>& counts);

            /**
             * @copydoc Histogram
             * @details This plots a histogram for each channel's tags.
             */
            static void Histogram(const DetectionReportList::const_iterator& start,
                           const DetectionReportList::const_iterator& end,
                           uint_fast16_t numBins,
                           PicoSeconds windowWidth,
                           ChannelHistograms& counts);

            /**
             * @brief FindPeak
             * Createa histogram of the data and find the highest count
             * @param sampleStart start of data to read
             * @param sampleEnd end of data to read
             * @return The centre of the peak as a percentage of the histogram
             */
            double FindPeak(DetectionReportList::const_iterator sampleStart,
                                  DetectionReportList::const_iterator sampleEnd) const;

            /**
             * @brief GetPeaks Runs FindPeak over a complete data set to produce a list of peaks
             * @param start Start point for data
             * @param end End point for data
             * @param[out] peaks The position in bins of the peaks
             * @param maximum The highest value found
             */
            void GetPeaks(const DetectionReportList::const_iterator& start,
                          const DetectionReportList::const_iterator& end,
                          std::vector<double>& peaks,
                          std::vector<double>::const_iterator& maximum);
        protected:
            /// The number of histogram bins to use when calculating drift
            BinID driftBins;
            /// Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
            const PicoSeconds slotWidth;
            /// The window used for calculating drift
            PicoSeconds driftSampleTime;
            /// processes items using available threads
            ProcessingQueue<double> workQueue;
        };

    } // namespace align
} // namespace cqp


