#pragma once
#include "Algorithms/Datatypes/Chrono.h"
#include "Algorithms/Datatypes/DetectionReport.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Drift class calculates drift based on detections
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
            AttoSecondOffset Calculate(const DetectionReportList::const_iterator& start,
                                            const DetectionReportList::const_iterator& end) const;

        protected: // methods

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
                           std::vector<uint64_t>& counts);

            /**
             * @brief FindPeak
             * Createa histogram of the data and find the highest count
             * @param sampleStart start of data to read
             * @param sampleEnd end of data to read
             * @return The time in picoseconds from the start of the data
             */
            double FindPeak(DetectionReportList::const_iterator sampleStart,
                                  DetectionReportList::const_iterator sampleEnd) const;

        protected:
            /// The number of histogram bins to use when calculating drift
            uint64_t driftBins;
            /// Picoseconds of time in which one qubit can be detected. slot width = frame width / number transmissions per frame
            const PicoSeconds slotWidth;
            /// The window used for calculating drift
            PicoSeconds driftSampleTime;
        };

    } // namespace align
} // namespace cqp


