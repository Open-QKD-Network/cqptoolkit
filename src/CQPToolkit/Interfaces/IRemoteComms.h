/*!
* @file
* @brief NullAlignment
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 04/02/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include <memory>
#include <grpc++/channel.h>
#include <grpc++/server_builder.h>

namespace cqp {
    /**
     * @brief The IRemoteComms interface for classes which can provide and/or use remote interfaces
     */
    class CQPTOOLKIT_EXPORT IRemoteComms
    {
    public:
        /**
         * @brief Connect
         * Notifies that a new channel is available, the class can now create stubs to remote objects
         * @param channel The channel to use.
         */
        virtual void Connect(std::shared_ptr<grpc::ChannelInterface> channel) = 0;

        /**
         * @brief Disconnect
         * The channel is being disconnected, all communication must stop.
         */
        virtual void Disconnect() = 0;

        /// Distructor
        virtual ~IRemoteComms() = default;
    };
}
