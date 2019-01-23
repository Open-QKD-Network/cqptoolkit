#pragma once
#include <cmath>

namespace cqp {

    /**
     * Divide two integer numbers and round to the nearest int
     * @tparam T Datatype for the numerator and the return
     * @tparam D Datatype for the denumerator
     * @param numerator number to devide
     * @param denumerator number to devide numerator by
     * @return result of division with rounding
     */
    template<typename T, typename D>
    T DivNearest(T numerator, D denumerator)
    {
        T result;
        if (numerator < T(0)) {
            result = (numerator - (denumerator/2) + 1) / denumerator;
        } else {
            result = (numerator + denumerator/2) / denumerator;
        }

        return result;
    }
}
