/*!
* @file
* @brief NullAlignment
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08/01/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "CQPToolkit/Interfaces/IAlignmentPublisher.h"
#include "Algorithms/Util/Provider.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include <Algorithms/Util/WorkerThread.h>
#include <grpc++/channel.h>
#include "CQPToolkit/Interfaces/IRemoteComms.h"

namespace cqp
{
    namespace align
    {

        /**
         * @brief The NullAlignment class
         * Pass through class for alignment, does not perform any actions on the incomming data, simply passes it to the listener
         */
        class CQPTOOLKIT_EXPORT NullAlignment : public Provider<IAlignmentCallback>,
        /* Interfaces */
            public virtual IDetectionEventCallback,
            public virtual IEmitterEventCallback,
            public virtual IRemoteComms,
            protected WorkerThread
        {
        public:
            NullAlignment() = default;

            ~NullAlignment() override
            {
                Disconnect();
            }

            /// @{
            /// @name IDetectionEventCallback Interface

            /**
             * @brief OnPhotonDetection
             * Photons have been received
             * @param report
             */
            void OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report) override;

            /// @}

            /// @copydoc IEmitterEventCallback::OnEmitterReport
            void OnEmitterReport(std::unique_ptr<EmitterReport> report) override;

            /// @{
            /// @name IRemoteComms interface

            /**
             * @brief Connect Connect to the other side to exchange alignment detail
             * @param channel
             */
            void Connect(std::shared_ptr<grpc::ChannelInterface> channel) override;

            /**
             * @brief Disconnect
             */
            void Disconnect() override;

            /// @}

            /**
             * @brief DoWork
             */
            void DoWork() override;

        protected:

            /// storage for incoming data
            std::queue<std::unique_ptr<QubitList>> receivedData;
            std::queue<std::unique_ptr<IntensityList>> receivedIntensities;
            /// connection to the other side
            std::shared_ptr<grpc::ChannelInterface> transmitter;

            /// our alignment sequence counter
            SequenceNumber seq = 0;

        };

    } // namespace align
} // namespace cqp


