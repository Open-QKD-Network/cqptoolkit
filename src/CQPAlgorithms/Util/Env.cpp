#include "Env.h"

namespace cqp {

    std::string GetEnvironmentVar(const std::string& key)
    {
        return std::string(getenv(key.c_str()));
    }

} // namespace cqp
