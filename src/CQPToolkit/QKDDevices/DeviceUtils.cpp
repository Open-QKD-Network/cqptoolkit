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
#include "DeviceUtils.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Util/Strings.h"
#include "CQPToolkit/Interfaces/IQKDDevice.h"

namespace cqp {

    remote::Side::Type DeviceUtils::GetSide(const URI& address)
    {
        remote::Side::Type result = remote::Side::Type::Side_Type_Any;
        std::string sideStr;
        if(address.GetFirstParameter("side", sideStr))
        {
            sideStr = ToLower(sideStr);
            if(sideStr == "alice")
            {
                result = remote::Side::Type::Side_Type_Alice;
            } else if(sideStr == "bob")
            {
                result = remote::Side::Type::Side_Type_Bob;
            }
        }
        return result;
    }

    std::string DeviceUtils::GetDeviceIdentifier(const URI& url)
    {
        using namespace std;
        string switchPort;
        string side;
        size_t bytesPerKey = DefaultBytesPerKey;

        url.GetFirstParameter(IQKDDevice::Parmeters::switchPort, switchPort);
        url.GetFirstParameter(IQKDDevice::Parmeters::side, side);
        url.GetFirstParameter(IQKDDevice::Parmeters::keybytes, bytesPerKey);

        std::string result = url.GetScheme() + "_" +
                             url.GetHost() + "_" +
                             std::to_string(url.GetPort()) + "_" +
                             switchPort + "_" + std::to_string(bytesPerKey) + "_" +
                             side;

        return result;
    }

} // namespace cqp
