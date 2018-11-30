/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPAlgorithms/Datatypes/Services.h"

namespace cqp
{
    /**
     * @brief The IServiceCallback class
     * Provides access to controllers discovered on the network
     */
    class CQPTOOLKIT_EXPORT IServiceCallback
    {
    public:
        /**
         * @brief ~IServiceCallback
         * destructor
         */
        virtual ~IServiceCallback() = default;

        /**
         * @brief OnServiceDetected
         * Called by service discovery every time there is a change in services
         * @param newServices Services which have not been seen before
         * @param deletedServices Services which have not been seen for a given timeout
         */
        virtual void OnServiceDetected(const RemoteHosts& newServices, const RemoteHosts& deletedServices) = 0;
    };
}
