#pragma once
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Util/RangeProcessing.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Offsetting class finds the alignment between two sets of loosly connected data
         */
        class ALGORITHMS_EXPORT Offsetting
        {
        public:
            /// The number of values to check in a data set
            static constexpr size_t DefaultSamples = 1000;

            /**
             * @brief Offsetting
             * @param samples The number of values to check in a data set
             */
            explicit Offsetting(size_t samples = DefaultSamples);

            /**
             * @brief The Confidence struct
             * Storage for the score and its offset
             */
            struct Confidence {
                /// The percentage of match between 0-1
                double value;
                /// The offset at which the confidence value was measured
                ssize_t offset;
            };

            /**
             * @brief HighestValue
             * Find the best match between the two datasets.
             * @param truth Values which are known to be true
             * @param validSlots The slots for The slot ids which the irregular values relate to
             * @param irregular The values which have an unknown validity and start offset
             * @param from The offset starting point
             * @param to The offset end point
             * @return The highest scoring offset and its confidence value
             */
            Confidence HighestValue(const QubitList& truth,  const std::vector<SlotID>& validSlots,
                              const QubitList& irregular, ssize_t from, ssize_t to);

            /**
             * @brief HighestValue
             * Find the best match between the two datasets.
             * @param markers Values which are known to be true, indexed by slot id
             * @param validSlots The slots for The slot ids which the irregular values relate to
             * @param irregular The values which have an unknown validity and start offset
             * @param from The offset starting point
             * @param to The offset end point
             * @return The highest scoring offset and its confidence value
             */
            Confidence HighestValue(const QubitsBySlot markers,
                                    const std::vector<SlotID>& validSlots,
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

            /**
             * @brief CompareValues
             * @todo This needs to be able to take a sparse list of true values
             * @param markers Sparse list of values known to be correct
             * @param validSlots The slot ids which the irregular values relate to
             * @param irregular The values which have an unknown validity and start offset
             * @param offset Shift the irregular comparison index.
             * @return The match confidence
             */
            double CompareValues(const QubitsBySlot markers,  const std::vector<uint64_t>& validSlots,
                                 const QubitList& irregular, ssize_t offset);
        protected:
            /// The number of values to check in a data set
            size_t samples;
            /// process the different offsets
            RangeProcessing<ssize_t> rangeWorker;
        };

    } // namespace align
} // namespace cqp


