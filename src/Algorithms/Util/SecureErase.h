#pragma once
#include "Algorithms/algorithms_export.h"
#include <vector>

namespace cqp {

    /// Clear a memory region such that it's contents cannot be recovered
    /// @param[in,out] data Address to start clearing
    /// @param[in] length Number of bytes to clear
    ALGORITHMS_EXPORT void SecureErase(void* data, unsigned int length);

    /// @copybrief SecureErase
    /// @tparam T The data type stored in the vector
    /// @param data data to clear
    template<typename T>
    void SecureErase(std::vector<T>& data)
    {
        SecureErase(data.data(), data.size() * sizeof(T));
    }

    /**
     * For use with STL elements which take a delete operator (such as unique_ptr)
     */
    template<class T>
    class SecureDeletor
    {
    public:
        void operator()(T* ptr) {
            SecureErase(ptr, sizeof (*ptr));
        }
    };

} // namespace cqp


