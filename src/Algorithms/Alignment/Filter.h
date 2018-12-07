#pragma once
#include "Algorithms/algorithms_export.h"
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include "Algorithms/Datatypes/Chrono.h"
#include "Algorithms/Datatypes/DetectionReport.h"

namespace cqp {
    namespace align {

        /**
         * @brief The Filter class
         */
        class ALGORITHMS_EXPORT Filter
        {
        public:
            /// Constructor
            Filter(double sigma = 5.0, size_t filterWidth = 21, double cutoff = 0.2, size_t stride = 25);

            void Isolate(const DetectionTimes& timeTags,
                         DetectionTimes::const_iterator& start,
                         DetectionTimes::const_iterator& end);

            /**
             * @brief Gaussian
             * Calculate a point on the Gaussian curve
             * @tparam T The type to use for calculations
             * @param sigma The standard deviation of the distribution
             * @param x The x axis
             * @return The y value of the point.
             * @details
             * \f$
             * G(x) = \frac{1}{ \sqrt{2 \pi \sigma ^2}} e^{- \frac{ x^2 }{ 2 \sigma ^ 2}}
             * \f$
             */
            template<typename T>
            static T Gaussian(T sigma, T x)
            {
                const T a = 1.0 / sqrt((2.0 * M_PI * pow(sigma, 2)));
                const T expVal = -1.0 * (pow(x, 2) / (2.0 * pow(sigma, 2)));
                return a * exp(expVal);

            } // Gaussian

            /**
             * @brief GaussianWindow1D
             * Create a 1D array of Gaussian distribution
             * @tparam T the type to calculate with
             * @param sigma The standard deviation of the distribution
             * @param width The width of the output array
             * @param peak The value of the centre point of the graph.
             *  The values will be scaled to this
             */
            template<typename T>
            static std::vector<T> GaussianWindow1D(T sigma, size_t width, T peak = 1.0)
            {
                using namespace std;
                // this is 1 if the width is odd
                const auto columnOffset = width % 2;
                // sifting the calculated value by 0.5 when the width is even as the peak is between the two middle values
                const auto xOffset = (1 - columnOffset) / 2.0;
                // the centre index of the output
                const auto mean = (width / 2) + columnOffset;
                // calculate the scale based on the peak using the true centre of the graph
                const T scale = 1.0 / Gaussian(sigma, 0.0) * peak;
                vector<T> result;
                result.resize(width);

                for(auto index = 0u; index < mean; index++)
                {
                    // calculate the point on the graph - this is offset by 0.5 for even widths.
                    const T kernelVal = Gaussian(sigma, xOffset + index) * scale;

                    // store the left side
                    result[mean - index - 1] = kernelVal;
                    // mirror the value
                    // for odd widths the centre will be a peak (columnOffset = 1)
                    // for even widths the two centre values will be the same
                    result[mean + index - columnOffset] = kernelVal;
                }

                return result;
            } // GaussianWindow1D

            /**
             *
             * @details
             * The data size must be >= filter size
             */
            template<typename T, typename DataIter, typename FilterIter>
            static void ConvolveValid(
                    const DataIter& inBegin, const DataIter& inEnd,
                    const FilterIter& filterBegin, const FilterIter& filterEnd,
                    T& result)
            {
                const auto dataSize = std::distance(inBegin, inEnd);
                const auto filterSize = std::distance(filterBegin, filterEnd);
                if(dataSize >= filterSize)
                {
                    result.resize(1ul + static_cast<size_t>(dataSize - filterSize));

                    for(size_t index = 0; index < result.size(); index++)
                    {
                        for(size_t filterIndex = 0; filterIndex < filterSize; filterIndex++)
                        {
                            // offset the start iterator and using it's value, multiply by the filter value
                            const auto& dataValue = *(inBegin + index + filterIndex);
                            const auto& filterValue = *(filterBegin + filterIndex);
                            result[index] += dataValue * filterValue;
                        }
                    }
                }
            }
        protected:
            const std::vector<double> filter;
            double cutoff;
            size_t stride;
        };

    } // namespace align
} // namespace cqp
