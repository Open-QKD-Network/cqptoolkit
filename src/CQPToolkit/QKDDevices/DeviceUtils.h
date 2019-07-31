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

    /**
     * @brief The DeviceUtils class a collection of helpers for deivces
     */
    class CQPTOOLKIT_EXPORT DeviceUtils
    {
    public:
        /// how to split up raw data into keys
        static const uint8_t DefaultBytesPerKey = 16;

        /**
         * @brief GetSide
         * extracts the side from the url
         * @param address to read
         * @return the side of the device or any if note specified in the url
         */
        static remote::Side::Type GetSide(const URI& address);

        /**
         * @brief GetDeviceIdentifier
         * extracts the device id from the url
         * @param url
         * @return a device id
         */
        static std::string GetDeviceIdentifier(const URI& url);

        /**
         * @brief ConfigToUri
         * @param config
         * @return populates a url from the device config
         */
        static URI ConfigToUri(const remote::DeviceConfig& config);
    };

} // namespace cqp


