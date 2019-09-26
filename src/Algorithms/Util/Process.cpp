/*!
* @file
* @brief Process
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Process.h"
#include <cstring>
#if defined(__unix__)
    #include <unistd.h>
    #include <sys/wait.h>
#elif defined(WIN32)
    #include <windows.h>
    #include <tchar.h>
#endif
#include <sys/types.h>
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Util/Strings.h"

namespace cqp
{
    enum Direction { Read = 0, Write = 1 };

    Process::~Process()
    {
        RequestTermination(true);
    }

    bool Process::Fork(const std::string& command, const std::vector<std::string>& args, int* stdIn, int* stdOut, int* stdErr)
    {
        bool result = false;
#if defined(__unix__)
        int pipeIn[2] {};
        int pipeOut[2] {};
        int pipeErr[2] {};
#elif defined(WIN32)
        HANDLE pipeIn[2] {};
        HANDLE pipeOut[2] {};
        HANDLE pipeErr[2] {};
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = true;
        saAttr.lpSecurityDescriptor = NULL;
        // TODO: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
#endif

        if(stdIn)
        {
#if defined(__unix__)
            if(::pipe(pipeIn) == -1)
#elif defined(WIN32)
            if (CreatePipe(&pipeIn[0], &pipeIn[1], &saAttr, 0) == 0)
#endif
            {
                LOGERROR("Failed to create pipe: " + Logger::GetLastErrorString());
            }
#if defined(WIN32)
            SetHandleInformation(pipeIn[1], HANDLE_FLAG_INHERIT, 0);
#endif
        }
        if(stdOut)
        {
#if defined(__unix__)
            if(::pipe(pipeOut) == -1)
#elif defined(WIN32)
            if (CreatePipe(&pipeOut[0], &pipeOut[1], &saAttr, 0) == 0)
#endif
            {
                LOGERROR("Failed to create pipe: " + Logger::GetLastErrorString());
            }

#if defined(WIN32)
            SetHandleInformation(pipeOut[0], HANDLE_FLAG_INHERIT, 0);
#endif

        }
        if(stdErr)
        {
#if defined(__unix__)
            if(::pipe(pipeErr) == -1)
#elif defined(WIN32)
            if (CreatePipe(&pipeErr[0], &pipeErr[1], &saAttr, 0) == 0)
#endif
            {
                LOGERROR("Failed to create pipe: " + Logger::GetLastErrorString());
            }
#if defined(WIN32)
            SetHandleInformation(pipeErr[0], HANDLE_FLAG_INHERIT, 0);
#endif
        }

        // ensure pipes are flushed
        ::fflush(nullptr);
#if defined(__unix__)
        // Clone the program into a new process
        int forkPid = ::fork();
#elif defined(WIN32)
        // TODO
        HANDLE forkPid{ 0 };
#endif

        if(forkPid == 0)
        {
            // we are now the new process
            try
            {
                using namespace std;
                const char**  c_args = new const char*[args.size() + 2];
                size_t argIndex = 0;
                // set the first element to the command name and increment the index
                c_args[argIndex++] = command.c_str();

                for(const auto& arg : args)
                {
                    // set the arg array to point to the argument string and increment the index
                    // a new string is needed otherwise each element points to the last string
                    c_args[argIndex++] = (new string(arg))->c_str();
                    // execv will destroy all the memory so these don't need to be cleaned up.
                }
                // the last element should be a null pointer
                c_args[argIndex++] = nullptr;

                if(stdIn)
                {
#if defined(__unix__)
                    // close the other end of the pipe
                    ::close(pipeIn[Write]);
                    ::dup2(pipeIn[Read], STDIN_FILENO);
                    ::close(pipeIn[Read]);
#elif defined(WIN32)
                    // TODO
#endif
                }

                if(stdOut)
                {
#if defined(__unix__)
                    // close the other end of the pipe
                    ::close(pipeOut[Read]);
                    ::dup2(pipeOut[Write], STDOUT_FILENO);
                    ::close(pipeOut[Write]);
#elif defined(WIN32)
                    // TODO
#endif
                }

                if(stdErr)
                {
#if defined(__unix__)
                    // close the other end of the pipe
                    ::close(pipeErr[Read]);
                    ::dup2(pipeErr[Write], STDERR_FILENO);
                    ::close(pipeErr[Write]);
#elif defined(WIN32)
                    // TODO
#endif
                }

#if defined(__unix__)
                // this is the new thread, launch the program
                ::execv(command.c_str(), const_cast<char* const*>(c_args));
#elif defined(WIN32)
                // TODO
#endif

                // exec only returns on error
                LOGERROR("Process execution failed : " + std::string(strerror(errno)));

                ::exit(errno);
            }
            catch(const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        else if(forkPid < 0)
        {
            LOGERROR("Fork failed : " + std::string(strerror(errno)));
        }
        else
        {
            // we are the original process, now with a child process
            pid = forkPid;
            if(stdIn)
            {
#if defined(__unix__)
                *stdIn = pipeIn[Write];
                ::close(pipeIn[Read]);
#elif defined(WIN32)
                // TODO
#endif
            }
            if(stdOut)
            {
#if defined(__unix__)
                *stdOut = pipeOut[Read];
                ::close(pipeOut[Write]);
#elif defined(WIN32)
                // TODO
#endif
            }
            if(stdErr)
            {
#if defined(__unix__)
                *stdErr = pipeErr[Read];
                ::close(pipeOut[Write]);
#elif defined(WIN32)
                // TODO
#endif
            }
            result = pid > 0;
        }

        return result;
    }

    bool Process::Start(const std::string& command, const std::vector<std::string>& args, int* stdIn, int* stdOut, int* stdErr)
    {
        bool result = false;
        RequestTermination(true);
        std::string cmdToRun;

        if(fs::Exists(command))
        {
            cmdToRun = command;
        }
        else
        {
            // try and find it in the path
            std::vector<std::string> paths;
            SplitString(getenv("PATH"), paths, fs::GetPathEnvSep());
            for(const auto& prefix : paths)
            {
                cmdToRun = prefix + fs::GetPathSep() + command;
                if(fs::Exists(cmdToRun))
                {
                    break;
                }

            }
        }

        if(fs::Exists(cmdToRun))
        {
            result = Fork(cmdToRun, args, stdIn, stdOut, stdErr);
        }
        else
        {
            LOGERROR("File not found: " + command);
        }
        return result;
    }

    bool Process::Running() const
    {
#if defined(__unix__)
        pid_t result = ::waitpid(pid, nullptr, WNOHANG);
#elif defined(WIN32)
        // TODO
        bool result = false;
#endif
        return result == 0;
    }

    void Process::RequestTermination(bool wait)
    {
        if(Running())
        {
#if defined(__unix__)
            ::kill(pid, SIGTERM);
#elif defined(WIN32)
            // TODO
            bool wait = false;
#endif
            if(wait)
            {
                WaitForExit();
            }
        }
    }

    int Process::WaitForExit()
    {
        int result = 0;
#if defined(__unix__)
        ::waitpid(pid, &result, 0);
        if(result != 0)
        {
            LOGERROR(::strerror(errno));
        }

        return WEXITSTATUS(result);

#elif defined(WIN32)
        // TODO
        return false;
#endif
    }

} // namespace cqp
