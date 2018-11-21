/*!
* @file
* @brief DeviceIO
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 7/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DeviceIO.h"
#include "CQPToolkit/Util/Logger.h"

namespace cqp
{
    namespace tunnels
    {

        bool DeviceIO::WaitUntilReady(std::chrono::milliseconds timeout) noexcept
        {
            using std::condition_variable;
            using std::unique_lock;

            unique_lock<std::mutex> lock(readyMutex);
            return readyCv.wait_for(lock, timeout, [&]()
            {
                return ready;
            });
        } // WaitUntilReady

        size_t DeviceIO::Put2(const unsigned char * inString, size_t length, int, bool)
        {
            size_t result = 0;
            if(length > 0 && Write(inString, length))
            {
                result = length;
            }

            return result;
        } // Put2

    } // namespace tunnels
} // namespace cqp

