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
#include "CQPToolkit/Alignment/Alignment.h"
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include <Algorithms/Util/WorkerThread.h>
#include <grpc++/channel.h>

namespace cqp {
    namespace align {

        /**
         * @brief The NullAlignment class
         * Pass through class for alignment, does not perform any actions on the incomming data, simply passes it to the listener
         */
        class CQPTOOLKIT_EXPORT NullAlignment : public Alignment,
                /* Interfaces */
                public virtual IDetectionEventCallback,
                public virtual IEmitterEventCallback,
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

            /**
             * @brief Connect Connect to the other side to exchange alignment detail
             * @param channel
             */
            void Connect(std::shared_ptr<grpc::Channel> channel) {
                transmitter = channel;
                Start();
            }

            /**
             * @brief Disconnect
             */
            void Disconnect()
            {
                Stop(true);
                receivedDataCv.notify_all();
                transmitter.reset();
            }

            /**
             * @brief DoWork
             */
            void DoWork() override;

        protected:

            /// storage for incoming data
            std::queue<std::unique_ptr<QubitList>> receivedData;
            std::shared_ptr<grpc::Channel> transmitter;
        };

    } // namespace align
} // namespace cqp


