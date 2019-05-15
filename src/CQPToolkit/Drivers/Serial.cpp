/*!
* @file
* @brief CQP Toolkit - Serial IO - common
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Drivers/Serial.h"

namespace cqp
{

    using namespace std;
    // Partial implementation, see Serial_Unix.cpp and Serial_Win32.cpp for platform dependent code

    bool Serial::Read(char& buffer, uint32_t& num)
    {
        return Read(&buffer, num);
    }

    bool Serial::Write(const char& buffer, uint32_t num)
    {
        return Write(&buffer, num);
    }

}
