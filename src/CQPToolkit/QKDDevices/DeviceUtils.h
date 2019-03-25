/*!
* @file
* @brief DeviceUtils
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 11/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{

    class URI;

    class CQPTOOLKIT_EXPORT DeviceUtils
    {
    public:
        /// how to split up raw data into keys
        static const size_t DefaultBytesPerKey = 16;

        static remote::Side::Type GetSide(const URI& address);

        static std::string GetDeviceIdentifier(const URI& url);

        static URI ConfigToUri(const remote::DeviceConfig& config);
    };

} // namespace cqp


