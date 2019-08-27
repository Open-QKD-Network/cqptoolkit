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
#pragma once
#include <IDQDevices/idqdevices_export.h>
#include <fstream>
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include <thread>
#include <atomic>
#include <condition_variable>

namespace cqp
{

    class IDQDEVICES_EXPORT ClavisKeyFile : public KeyPublisher
    {
    public:
        ClavisKeyFile(const std::string& filename);
        ~ClavisKeyFile();
    protected:
        void WatchKeyFile(const std::string filename);
        void ReadKeys(const std::string& filename, size_t& fileOffset);

    protected: // members
        std::atomic_bool keepGoing{true};
        std::thread reader;
        int watchFD = -1;
    };

} // namespace cqp


