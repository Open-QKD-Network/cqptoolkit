#pragma once
#include <string>

namespace cqp {

    /**
     * @brief GetEnvironmentVar
     * @param key Name of environment variable
     * @return The value of the environment variable
     */
    ALGORITHMS_EXPORT std::string GetEnvironmentVar(const std::string& key);

} // namespace cqp


