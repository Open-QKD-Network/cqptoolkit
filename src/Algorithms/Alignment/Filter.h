#pragma once
#include "Algorithms/algorithms_export.h"
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <map>
#include "Algorithms/Datatypes/Chrono.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include <functional>

namespace cqp {
    namespace align {

        /**
         * @brief The Filter class
         */
        class ALGORITHMS_EXPORT Filter
        {
        public:
            static constexpr double DefaultSigma = 5.0;
            static constexpr size_t DefaultFilterWidth = 21;
            static constexpr double DefaultCutoff = 0.2;
            static constexpr size_t DefaultStride = 25;

            /** Constructor
            * @param sigma value for the Gaussian filter
            * @param filterWidth number of elements for the Gaussian filter
            * @param cutoff The signal level which signifies a valid transmission as a percentage (0 - 1)
            * @param stride How many elements to reduce the data set by when detecting the transmission
            */
            Filter(double sigma = DefaultSigma, size_t filterWidth = DefaultFilterWidth,
                   double cutoff = DefaultCutoff, size_t stride = DefaultStride);

            /**
             * @brief Isolate
             * Find the start and end of transmission by looking for an increase in detections
             * @param timeTags Raw times of detections
             * @param[out] start The start of detections
             * @param[out] end The end of detections
             */
            bool Isolate(const DetectionReportList& timeTags,
                         DetectionReportList::const_iterator& start,
                         DetectionReportList::const_iterator& end);


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
             * @brief
             * Perform a "valid" convolution to the data using the filter by multiplying the two
             * arrays.
             * Only elements which the filter can be applied to are returned
             * Filter:        |****|
             * Data:   |-----------|
             * Result: |=======|
             *                  ^^^ These elements cannot be completely convolved
             * so are not returned in the result.
             * @details
             * The data size must be >= filter size
             * @tparam T The type of the result
             * @param inBegin The first element of data
             * @param inEnd The element past the last data element
             * @param filterBegin The first element of the filter
             * @param filterEnd The element pas the last filter element
             * @param convolved The storage for the result. It will be resized to fit.
             */
            template<typename T, typename DataIter, typename FilterIter>
            static bool ConvolveValid(
                    const DataIter& inBegin, const DataIter& inEnd,
                    const FilterIter& filterBegin, const FilterIter& filterEnd,
                    T& convolved)
            {
                bool result = false;
                const auto dataSize = std::distance(inBegin, inEnd);
                const auto filterSize = std::distance(filterBegin, filterEnd);
                if(dataSize >= filterSize)
                {
                    convolved.resize(1ul + static_cast<size_t>(dataSize - filterSize));

                    for(size_t index = 0; index < convolved.size(); index++)
                    {
                        for(ssize_t filterIndex = 0; filterIndex < filterSize; filterIndex++)
                        {
                            // offset the start iterator and using it's value, multiply by the filter value
                            const auto& dataValue = *(inBegin + index + filterIndex);
                            const auto& filterValue = *(filterBegin + filterIndex);
                            convolved[index] += dataValue * filterValue;
                        }
                    }
                    result = true;
                }
                return result;
            }

            /**
             * Find the edges of a noisy square wave using a binary search.
             * This will find a transison from high to low with the default less than comparitor
             * or a low to high edge with the greater than comaritor
             * @verbatim
             * |        ##  #####
             * |       #  ##
             * |----- # --------- cutoff
             * | ##  #
             * |#  ##
             * |_________________
             *  ^     ^          ^
             *  Start |          End
             *        ` Edge detected
             * @endverbatim
             * @note The result is undefined if the data contains more than one edge in the search direction.
             * @tparam T The data storage and cutoff type
             * @tparam Iter The iterator type for a collection of T
             * @param start Start the search from here
             * @param end End of search region
             * @param cutoff The transision value for the edge to be considered
             * @param[out] edge The edge if found, otherwise it will equal end
             * @param comparator The comparsion operator to use for detecting the edge condition.
             * The default < will detect a rising edge, use std::greater<T>() to detect a falling edge
             * @return true if the edge was found.
             */
            template<typename T, typename Iter>
            static bool FindEdge(Iter start, Iter end, T cutoff,
                          Iter& edge,
                          std::function<bool (const T&, const T&)> comparator = std::less<T>())
            {
                ssize_t lowerBound = 0;
                ssize_t upperBound = std::distance(start, end);
                auto searchIndex = upperBound / 2;
                bool result = false;
                edge = end;

                while((start + searchIndex) < end)
                {
                    if(comparator(*(start + searchIndex), cutoff))
                    {
                        // found a lower point, look left for the edge
                        upperBound = searchIndex;
                    } else {
                        // the value to the left is assumed to be no good
                        lowerBound = searchIndex;
                    }
                    // set the search point to half way between the two bounds
                    searchIndex = lowerBound + (upperBound - lowerBound) / 2;
                    if(lowerBound >= upperBound - 1)
                    {
                        // the bounds have meet store the result
                        edge = start + upperBound;
                        result = true;
                        break; // stop searching
                    } // if bounds meet
                } // while

                return result;
            } // FindEdge

        protected:
            /// The filter to apply
            const std::vector<double> filter;
            /// The signal level which signifies a valid transmission as a percentage (0 - 1)
            double cutoff;
            /// How many elements to reduce the data set by when detecting the transmission
            size_t stride;
        };

    } // namespace align
} // namespace cqp
