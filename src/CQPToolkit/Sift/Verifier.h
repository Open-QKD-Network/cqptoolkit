/*!
* @file
* @brief SiftReceiver
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Sift/SiftBase.h"
#include "QKDInterfaces/ISift.grpc.pb.h"
#include <CQPToolkit/Interfaces/IEmitterEventPublisher.h>

namespace cqp
{
    namespace sift
    {

        /// Accepts data inherently aligned data from the emitter and responds to requests from the
        /// Receiver to provide basis information and discard undetected qubits
        class CQPTOOLKIT_EXPORT Verifier : public virtual cqp::IEmitterEventCallback,
            public SiftBase, public remote::ISift::Service
        {
        public:

            /**
             * @brief SiftReceiver
             * Constructor
             */
            Verifier();

            ///@{
            /// @name remote::ISift interface

            /**
             * @copydoc remote::ISift::VerifyBases
             * @param context Connection details from the server
             * @return true on success
             * @details
             * @startuml VerifyBasesBehaviour
             * participant BB84Sifter
             * [-> BB84Sifter : VerifyBases
             * activate BB84Sifter
             *      BB84Sifter -> BB84Sifter : ProcessStates
             * [<-- BB84Sifter : ReturnResults
             * deactivate BB84Sifter
             * @enduml
             */
            grpc::Status VerifyBases(grpc::ServerContext* context, const remote::BasisBySiftFrame* request, remote::AnswersByFrame* response) override;

            ///@}
            /// @{
            /// @name IEmitterEventCallback interface

            /// @copydoc cqp::IEmitterEventCallback::OnEmitterReport
            void OnEmitterReport(std::unique_ptr<cqp::EmitterReport> report) override;
            ///@}
            ///
            /// @{
            /// @name IRemoteComms interface

            /**
             * @brief Connect
             * @param channel channel to the other sifter
             */
            void Connect(std::shared_ptr<grpc::ChannelInterface> channel) override;

            /**
             * @brief Disconnect
             */
            void Disconnect() override;

            ///@}

        protected:

            using EmitterStateList = std::map<SequenceNumber, std::unique_ptr<EmitterReport>>;
            void PublishStates(EmitterStateList::const_iterator start, EmitterStateList::const_iterator end,
                               const remote::AnswersByFrame& answers);

            /// How long to wait for incomming data
            std::chrono::milliseconds receiveTimeout {500};

            /// a mutex for use with collectedStatesCv
            std::mutex statesMutex;
            /// used for waiting for new data to arrive
            std::condition_variable statesCv;

            /// Data that has accumulated
            EmitterStateList collectedStates;
        }; // SiftReceiver

    } // namespace Sift
} // namespace cqp
