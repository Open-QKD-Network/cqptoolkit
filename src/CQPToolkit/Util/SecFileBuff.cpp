/*!
* @file
* @brief SecFileBuff
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/10/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SecFileBuff.h"
#include "CQPAlgorithms/Logging/Logger.h"

#if defined (__linux)
    #include <unistd.h>
#else
    #if defined (_WIN32)
        #include <io.h>
    #endif
#endif

namespace cqp
{

    SecFileBuff::SecFileBuff(FILE_HANDLE fd, size_t size) : fileDescriptor(fd), bufferSize(size),
        //ensure there's enough space to store incoming data based on the maximum packet size
        underflowBuffer(size), overflowBuffer(size)
    {
        // Sec block is an array, not a vector, end points to the end of the allocated block
        // set the get pointer's start point to the end - ie, there's no data so the first call to get
        // will cause an underflow
        setg(underflowBuffer.begin(), underflowBuffer.end(), underflowBuffer.end());
        // set the put pointer to the empty buffer, overflow will be called when it is full.
        setp(overflowBuffer.begin(), overflowBuffer.end());
    }

    SecFileBuff::int_type cqp::SecFileBuff::underflow()
    {
        using namespace std;
        int_type result = traits_type::eof();
        try
        {

            ssize_t bytesRead = read(fileDescriptor, underflowBuffer.BytePtr(), underflowBuffer.size());
            if(bytesRead > 0)
            {
                // tell the buffer how many items we've stored in it
                underflowBuffer.resize(static_cast<size_t>(bytesRead));
                // change the pointers to reference the data
                setg(underflowBuffer.begin(), underflowBuffer.begin(), underflowBuffer.end());
                // retrieve the last character and return it (no idea why)
                result = *(underflowBuffer.end() - 1);
            }
        }
        catch (const exception& e)
        {
            LOGERROR(e.what());
        }

        return result;
    }

    SecFileBuff::int_type cqp::SecFileBuff::overflow(int_type __c)
    {
        using namespace std;
        int_type result = traits_type::eof();
        try
        {
            ssize_t bytesWritten = 0;
            ssize_t bytesToWrite = pptr() - pbase();
            if(bytesToWrite > 0)
            {
                ssize_t numBytes = 0;
                while(numBytes >= 0 && bytesWritten < bytesToWrite)
                {
                    // using a walking pointer, write the next set of bytes
                    numBytes = write(fileDescriptor, pptr() + bytesWritten, static_cast<size_t>(bytesToWrite - bytesWritten));
                    bytesWritten += numBytes;
                }
            }

            if(bytesWritten == bytesToWrite)
            {
                // SecBlock is an array not a vector, it doesn't store how many elements are active,
                // just clean it out after we're done
                underflowBuffer.CleanNew(underflowBuffer.size());
                // change the pointers to reference the empty space
                setp(underflowBuffer.begin(), underflowBuffer.end());
                // retrieve the last character and return it (no idea why)
                result = traits_type::not_eof(__c);
            }
        }
        catch (const exception& e)
        {
            LOGERROR(e.what());
        }

        return result;
    }

} // namespace cqp
