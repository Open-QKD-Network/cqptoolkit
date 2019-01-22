#pragma once
#include "Algorithms/Datatypes/Qubits.h"

namespace cqp {
    namespace align {

        class Offsetting
        {
        public:
            static constexpr double DefaultRejection = 0.4;
            static constexpr double DefaultAcceptance = 0.7;
            static constexpr size_t DefaultMinTests = 100;
            static constexpr size_t DefaultSamples = 1000;

            /**
             * @brief Offsetting
             * @param rejectionThreshold Comparison consistentlently below this will be rejected
             * @param minTests The minimum number of comparisons to make before rejecting/accepting
             */
            Offsetting(size_t samples = DefaultSamples,
                       double rejectionThreshold = DefaultRejection,
                       double acceptanceThreshold = DefaultAcceptance,
                       size_t minTests = DefaultMinTests);

            /**
             * @brief CompareValues
             * @param truth
             * @param irregular
             * @param offset Shift the irregular comparison index.
             * @return The match confidence
             */
            double CompareValues(const QubitList& truth,  const std::vector<uint64_t>& validSlots,
                                 const QubitList& irregular, ssize_t offset);
        protected:
            double rejectionThreshold;
            double acceptanceThreshold;
            size_t minTests;
            size_t samples;
        };

    } // namespace align
} // namespace cqp


