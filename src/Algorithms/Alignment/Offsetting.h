#pragma once
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Util/RangeProcessing.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Offsetting class finds the alignment between two sets of loosly connected data
         */
        class Offsetting
        {
        public:
            /// Match rates below this will be rejected
            static constexpr double DefaultRejection = 0.6;
            /// Numer of tests before allowing an immediate accept/reject
            static constexpr size_t DefaultMinTests = 100;
            /// The number of values to check in a data set
            static constexpr size_t DefaultSamples = 1000;

            /**
             * @brief Offsetting
             * @param samples The number of values to check in a data set
             * @param rejectionThreshold Comparison consistentlently below this will be rejected
             * @param acceptanceThreshold Match rates above this will be accepted
             * @param minTests The minimum number of comparisons to make before rejecting/accepting
             */
            Offsetting(size_t samples = DefaultSamples,
                       double rejectionThreshold = DefaultRejection,
                       size_t minTests = DefaultMinTests);

            struct Confidence {
                double value;
                ssize_t offset;
            };

            Confidence HighestValue(const QubitList& truth,  const std::vector<uint64_t>& validSlots,
                              const QubitList& irregular, ssize_t from, ssize_t to);

            /**
             * @brief CompareValues
             * @todo This needs to be able to take a sparse list of true values
             * @param truth Values known to be correct
             * @param validSlots The slot ids which the irregular values relate to
             * @param irregular The values which have an unknown validity and start offset
             * @param offset Shift the irregular comparison index.
             * @return The match confidence
             */
            double CompareValues(const QubitList& truth,  const std::vector<uint64_t>& validSlots,
                                 const QubitList& irregular, ssize_t offset);
        protected:
            /// Match rates below this will be rejected
            double rejectionThreshold;
            /// Numer of tests before allowing an immediate accept/reject
            size_t minTests;
            /// The number of values to check in a data set
            size_t samples;
            /// process the different offsets
            RangeProcessing<ssize_t> rangeWorker;
        };

    } // namespace align
} // namespace cqp


