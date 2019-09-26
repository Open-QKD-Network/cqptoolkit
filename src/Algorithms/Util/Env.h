#pragma once
#include <string>
#include "Algorithms/algorithms_export.h"

namespace cqp {

    /**
     * @brief GetEnvironmentVar
     * @param key Name of environment variable
     * @return The value of the environment variable
     */
    ALGORITHMS_EXPORT std::string GetEnvironmentVar(const std::string& key);

    /**
     * @brief ApplicationName
     * @return The current application name
     */
    ALGORITHMS_EXPORT std::string ApplicationName();

    /**
     * @brief ProcessId
     * @return The current process id
     */
    ALGORITHMS_EXPORT int ProcessId();
} // namespace cqp


