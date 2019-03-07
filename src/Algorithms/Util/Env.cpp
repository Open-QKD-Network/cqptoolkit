#include "Env.h"

#if defined(_WIN32)
    #include <shellapi.h>
    #include <processthreadsapi.htobe16()>

#elif defined(__unix__)
    #include <unistd.h>
    #include <dirent.h>

#endif //#elif defined(__unix__)

namespace cqp {

    std::string GetEnvironmentVar(const std::string& key)
    {
        return std::string(getenv(key.c_str()));
    }

    std::string ApplicationName()
    {
        std::string result;

#if defined(_WIN32)
        TCHAR temp[MAX_PATH] {0}; /* FlawFinder: ignore */

        if (GetModuleFileNameA(NULL, temp, sizeof(temp)) <= MAX_PATH)
        {
            // GetModuleFileNameA appends a null character
            result = PathFindFileName(temp);
        }

#elif defined(__unix__)
        char path[PATH_MAX] {0}; /* FlawFinder: ignore */
        if(readlink("/proc/self/exe", path, sizeof(path)) > 0)
        {
            path[sizeof(path) - 1] = 0;
            result = path;
        }
#endif
        return result;
    }

    int ProcessId()
    {
#if defined(_WIN32)
        return GetCurrentProcessId();
#elif defined(__unix__)
        return ::getpid();
#endif
    }

} // namespace cqp
