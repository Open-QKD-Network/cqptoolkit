/*!
* @file
* @brief CQP Toolkit - File utility functions
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Util/Util.h"
#include <string>
#include <sstream>
#include "FileIO.h"
#include <fstream>
#include "CQPToolkit/Util/Process.h"
#include <string.h>
#include <algorithm>

#if defined(_WIN32)
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
    #include <shlwapi.h>
    #include <io.h>

#elif defined(__unix__)
    #include <stdlib.h>
    #include <linux/limits.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <glob.h>
    #include <libgen.h>

#endif //#elif defined(__unix__)

namespace cqp
{
    namespace fs
    {

        std::string GetHomeFolder()
        {
#if defined(_WIN32)
            TCHAR temp[MAX_PATH];

            // Get the folder for the current users my documents folder
            SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, temp);
            std::string result = std::string(temp);

#elif defined(__unix__)

            std::string result = std::string(getenv("HOME"));
#endif
            return result;
        }

        std::string GetPathSep()
        {
#if defined(WIN32)
            return "\\";
#else
            return "/";
#endif
        }

        /// The name of the browser used to open a link
        static std::string browserUsedLast = "";

        bool OpenURL(const std::string& url)
        {
            using namespace std;
            // Search for some environment variables which might tell us how to open a browser
            bool browserFound = false;
#if defined(WIN32)
            browserFound = ShellExecute(NULL, NULL, url.c_str(), 0, 0, SW_SHOWDEFAULT) > (HINSTANCE)(32); // why 32? see: https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx
#elif defined(__unix__)
            vector<string> args;
            args.push_back(url);
            string possibleBrowsers[] =
            {
                browserUsedLast,
                GetEnvironmentVar("BROWSER"),
                "x-www-browser",
                "start"
                "xdg-open",
                url
            };

            for (string possibleBrowser : possibleBrowsers)
            {
                if (possibleBrowser != "")
                {
                    try
                    {
                        Process browserHandle;
                        browserFound = browserHandle.Start(possibleBrowser, args);
                        browserUsedLast = possibleBrowser;
                        break; // for
                    }
                    catch (const exception&) {}
                }
            }
#else
            LOGERROR("OpenURL unimplemented for this OS");
#endif
            return browserFound;
        }


        std::string GetApplicationName()
        {
            std::string result = "";

#if defined(_WIN32)
            TCHAR temp[MAX_PATH];

            if (GetModuleFileNameA(NULL, temp, MAX_PATH) <= MAX_PATH)
            {
                result = PathFindFileName(temp);
            }

#elif defined(__unix__)
            char *path = new char[PATH_MAX];
            if (path != nullptr)
            {
                *path = {0};

                if(readlink("/proc/self/exe", path, PATH_MAX) > 0)
                {
                    result = TO_STRING(path);
                }
                delete[](path);
            }
#endif
            return result;
        }

        bool Exists(const std::string& filename)
        {
            bool result = false;

#if defined(_WIN32)
            struct stat buffer;
            result = (_stat (filename.c_str(), &buffer) == 0);

#elif defined(__unix__)
            struct stat buffer;
            result = (stat (filename.c_str(), &buffer) == 0);
#endif
            return result;
        }

        bool ReadEntireFile(const std::string& filename, std::string& output, size_t limit)
        {
            bool result = false;
            std::ifstream inFile(filename, std::ios::in | std::ios::binary);
            if (inFile)
            {
                inFile.seekg(0, std::ios::end);
                output.resize(std::min(static_cast<size_t>(inFile.tellg()), limit));
                inFile.seekg(0, std::ios::beg);
                inFile.read(&output[0], static_cast<long>(output.size()));
                inFile.close();
                result = true;
            }
            return result;
        }

        bool WriteEntireFile(const std::string& filename, const std::string& contents)
        {
            bool result = false;
            std::ofstream outFile(filename, std::ios::out | std::ios::binary);
            if (outFile)
            {
                outFile.write(contents.data(), static_cast<long>(contents.length()));
                outFile.close();
                result = true;
            }
            return result;
        }

        bool IsDirectory(const std::string& path)
        {
            bool result = false;
#if defined (__unix__)
            struct stat st {};
            if(stat(path.c_str(), &st) == 0)
            {
                result = S_ISDIR(st.st_mode);
            }
#elif defined(WIN32)
            result = (GetFileAttributes(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
            return result;
        }

        std::vector<std::string> ListChildren(const std::string& path)
        {
            std::vector<std::string> result;

#if defined (__unix__)
            DIR* dir = opendir(path.c_str());
            struct dirent* child {};
            while((child = readdir(dir)) != nullptr)
            {
                result.push_back(child->d_name);
            }
            closedir(dir);
#elif defined(WIN32)
            std::string pattern(path);
            pattern.append("\\*");
            WIN32_FIND_DATA data;
            HANDLE hFind;
            if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    result.push_back(data.cFileName);
                }
                while (FindNextFile(hFind, &data) != 0);
                FindClose(hFind);
            }
#endif
            return result;
        }

        std::vector<std::string> FindGlob(const std::string& search)
        {
            std::vector<std::string> result;
#if defined (__unix__)
            glob_t found {};
            int globResult = glob(search.c_str(), GLOB_TILDE, nullptr, &found);
            if(globResult == 0)
            {
                for(size_t index = 0; index < found.gl_pathc; index++)
                {
                    result.push_back(found.gl_pathv[index]);
                }
            }
            globfree(&found);
#elif defined(WIN32)
            std::string pattern(search);
            WIN32_FIND_DATA data;
            HANDLE hFind;
            if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    result.push_back(data.cFileName);
                }
                while (FindNextFile(hFind, &data) != 0);
                FindClose(hFind);
            }
#endif
            return result;
        }

        std::string GetCurrentPath()
        {
            std::string result;
            char* c_result = ::getcwd(nullptr, 0);
            result = c_result;
            free(c_result);
            return result;
        }

        std::string Parent(const std::string& path)
        {
            std::string result;
#if defined(__unix__)
            // according to https://linux.die.net/man/3/dirname, dirname
            // could return pointers to statically allocated memory which
            // may be overwritten by subsequent calls. Alternatively,
            // they may return a pointer to some part of path!
            // so dont free anything

            result = ::dirname(const_cast<char*>(path.c_str()));
#elif defined(WIN32)
            PathRemoveFileSpec
#endif
            return result;
        }

        bool CanWrite(const std::string& path)
        {
            return ::access(path.c_str(), W_OK) == 0;
        }

        bool CreateDirectory(const std::string& path)
        {
            bool result = false;
#if defined (__unix__)
            result = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#elif defined(WIN32)
            result = ::CreateDirectory(path.c_str(), nullptr);
#endif
            return result;
        }

        std::string MakeTemp(bool directory)
        {
            char name[] = "/tmp/temp.XXXXXX";
            if(directory)
            {
                // name is modified to contain the folder created
                // ignore the return value which is just a pointer to name
                (void) ::mkdtemp(name);
            }
            else
            {
                int fd = ::mkstemp(name);
                if(fd)
                {
                    // mkstemp creates the file and opens it to stop race conditions
                    ::close(fd);
                }
            }
            return name;
        }

        bool Delete(const std::string& path)
        {
            return ::remove(path.c_str()) == 0;
        }

        std::string BaseName(const std::string& path)
        {
            std::string result;
#if defined(__unix__)
            // according to https://linux.die.net/man/3/dirname, dirname
            // could return pointers to statically allocated memory which
            // may be overwritten by subsequent calls. Alternatively,
            // they may return a pointer to some part of path!
            // so dont free anything

            result = ::basename(const_cast<char*>(path.c_str()));
#elif defined(WIN32)
            PathRemoveFileSpec
#endif
            return result;
        }

    } // namespace fs
} // namespace cqp

