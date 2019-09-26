/*!
* @file
* @brief ClavisKeyFile
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/8/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ClavisKeyFile.h"
#include <array>
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/FileIO.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <future>
#include <assert.h>
#if defined(__unix__)
#include <sys/inotify.h>
#include <unistd.h>
#include <linux/limits.h>
#elif defined(WIN32)
#include <io.h>
#endif

namespace cqp
{

    struct KeyEntry
    {
        UUID id;
        std::array<unsigned char, 32> key;
    };

    union KeyMapped
    {
        KeyMapped() {}

        KeyEntry values;
        std::array<char, sizeof (KeyEntry)> bytes;
    };

    ClavisKeyFile::ClavisKeyFile(const std::string& filename)
    {
        using namespace std;
        assert(sizeof (KeyEntry) == sizeof (KeyMapped));
        reader = thread(&ClavisKeyFile::WatchKeyFile, this, fs::FullPath(filename));
    }

    ClavisKeyFile::~ClavisKeyFile()
    {
        keepGoing = false;
        if(watchFD != -1)
        {
            ::close(watchFD);
        }

        if(reader.joinable())
        {
            reader.join();
        }
    }

    void ClavisKeyFile::WatchKeyFile(const std::string filename)
    {
        using namespace std;
        size_t fileOffset = 0;

#if defined(__unix__)
        watchFD = ::inotify_init();
        if(watchFD == -1)
        {
            LOGERROR("Failed to add watch for " + filename);
            keepGoing = false;
        }

        while(keepGoing)
        {

            // wait for the file
            if(!fs::Exists(filename))
            {
                LOGTRACE("Waiting for file creation");
                auto dir = fs::Parent(filename);
                if(dir.empty())
                {
                    dir = ".";
                }

                const auto dirWatchId = ::inotify_add_watch(watchFD, dir.c_str(), IN_CREATE);
                if(dirWatchId > 0)
                {
                    // read the changes until the file is ready
                    bool ready = false;
                    char buffer[(sizeof (struct inotify_event) + PATH_MAX) * 1024];

                    do
                    {
                        auto bytes = ::read(watchFD, buffer, sizeof (buffer));
                        if(bytes >= static_cast<ssize_t>(sizeof (struct inotify_event)))
                        {
                            auto offset = 0u;
                            while(!ready && offset < bytes)
                            {
                                struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[offset]);
                                // we're done if a file has been created with the right name
                                ready = event->mask & IN_CREATE && fs::Exists(filename);
                                offset += sizeof (struct inotify_event) + event->len;
                            }
                        }
                        else
                        {
                            LOGERROR("failed to read from directory watch");
                            //break; // while
                        }
                    }
                    while(keepGoing && !ready);

                    ::inotify_rm_watch(watchFD, dirWatchId);
                }
                else
                {
                    LOGERROR("Failed to add watch on parent directory");
                }
            } // if file missing

            struct ::stat fileStat;

            // if the file exists but is too small, wait for more writes
            while(keepGoing && ::stat(filename.c_str(), &fileStat) == 0)
            {
                if(fileStat.st_size  > 0 && static_cast<size_t>(fileStat.st_size) >= fileOffset + sizeof(KeyEntry))
                {
                    ReadKeys(filename, fileOffset);
                }
                // now we will have read the entire file
                // watch for writes to the file.
                auto fileWatchId = ::inotify_add_watch(watchFD, filename.c_str(), IN_MODIFY);

                char buffer[(sizeof (struct inotify_event) + PATH_MAX) * 1024];
                // block until something happens
                LOGTRACE("Waiting for file write");
                bool ready = false;
                do
                {
                    auto bytes = ::read(watchFD, buffer, sizeof (buffer));
                    if(bytes >= static_cast<ssize_t>(sizeof (struct inotify_event)))
                    {
                        auto offset = 0u;
                        while(!ready && offset < bytes)
                        {
                            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[offset]);
                            ready = event->mask & IN_MODIFY;
                            offset += sizeof (struct inotify_event) + event->len;
                        }
                    }
                    else
                    {
                        LOGERROR("failed to read from file watch");
                        break; // while
                    }
                }
                while(keepGoing && !ready);

                LOGTRACE("File watch woke up");
                ::inotify_rm_watch(watchFD, fileWatchId);
            } // while file exists

        } // outer keep going loop
#elif defined(WIN32)
LOGERROR("TODO");
#endif

        if(watchFD != -1)
        {
            close(watchFD);
            watchFD = -1;
        }
    } // ReadKeys


    void ClavisKeyFile::ReadKeys(const std::string& filename, size_t& fileOffset)
    {
        using namespace std;
        LOGTRACE("Opening key file");
        auto sourceFile = ifstream(filename, ios::in | ios::binary);

        // if the file can be open, see how many keys we can get out of it
        if(sourceFile && sourceFile.good())
        {
            LOGTRACE("Reading keys");
            size_t keysToGet = 0;
            struct ::stat fileStat;
            // try and get the file size
            if(::stat(filename.c_str(), &fileStat) == 0)
            {
                // check that the file hasn't been reset
                // if we're trying to read past the end of the file something has gone wrong
                if(fileStat.st_size >= static_cast<int64_t>(fileOffset))
                {
                    // calculate how many fill key records are yet to be read
                    keysToGet = (static_cast<size_t>(fileStat.st_size) - fileOffset) /
                                sizeof(KeyEntry);
                    LOGDEBUG("File has " + to_string(keysToGet) + " unread keys");
                }
                else
                {
                    LOGERROR("Current offset past EOF - ABORT");
                    keepGoing = false;
                }
            } // if stat succeded

            KeyMapped nextKey;
            // read the keys
            auto keyData = make_unique<KeyList>();

            for(auto count = 0u; count < keysToGet; count++)
            {
                sourceFile.read(nextKey.bytes.data(), sizeof(KeyEntry));
                keyData->push_back({nextKey.values.key.begin(), nextKey.values.key.end()});
            }

            if(!keyData->empty())
            {
                Emit(&IKeyCallback::OnKeyGeneration, move(keyData));
            }

            if(sourceFile.eof())
            {
                sourceFile.close();
            }
            // update the counter for our current location
            fileOffset += keysToGet * sizeof (KeyMapped);
        } // if file good
    } // ClavisKeyFile::ReadKeys

} // namespace cqp
