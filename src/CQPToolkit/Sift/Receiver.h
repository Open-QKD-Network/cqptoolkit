/*!
* @file
* @brief BB84
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/WorkerThread.h"
#include "SiftBase.h"
#include "QKDInterfaces/ISift.grpc.pb.h"
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"

namespace cqp
{
    namespace sift
    {

        /**
         * @brief Takes sparse indexed qubits from a detector and sifts the data by calling the Verifier
         */
        class CQPTOOLKIT_EXPORT Receiver : public virtual cqp::IDetectionEventCallback,
            protected WorkerThread, public SiftBase
        {
        public:
            /**
             * @brief BB84Sifter
             * Constructor
             * @param framesBeforeVerify How many frames to collect before verifying data.
             */
            explicit Receiver(unsigned int framesBeforeVerify = 1);

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

            ///@{
            /// @name IDetectionEventCallback interface

            /// @copydoc cqp::IDetectionEventCallback::OnPhotonReport
            void OnPhotonReport(std::unique_ptr<cqp::ProtocolDetectionReport> report) override;

            /// @}

            /**
             * @brief ~BB84Sifter
             * Destructor
             */
            ~Receiver() override;

        protected:
            ///@{
            /// @name WorkerThread Override

            /**
             * @brief DoWork
             * @details
             * @startuml TransmitterDoWorkBehaviour
             * [-> BB84Sifter : DoWork
             * activate BB84Sifter
             *      BB84Sifter -> BB84Sifter : WaitForData
             *      BB84Sifter -> BB84Sifter : ProcessStates
             *      BB84Sifter -> ISift : VerifyBases
             *      BB84Sifter -> BB84Sifter : Emit(validData)
             * deactivate BB84Sifter
             * @enduml
             */
            void DoWork() override;

            /// @}

            /**
             * @brief ValidateIncomming
             * Checks if the data is ready to be used
             * Should only be called from within condition_variable.wait()
             * @param firstSeq
             * @return true when there is data to process
             */
            bool ValidateIncomming(SequenceNumber firstSeq) const;

        protected:
            using StatesList = std::map<SequenceNumber, std::unique_ptr<ProtocolDetectionReport>>;

            void PublishStates(StatesList::const_iterator start, StatesList::const_iterator end,
                               const remote::AnswersByFrame& answers);
            /// The other side to communicate with during sifting
            std::unique_ptr<remote::ISift::Stub> verifier;
            /// How long to wait for new data before checking if the thread should be stopped
            const std::chrono::seconds threadTimeout {1};
            /// How many aligned frames to receive before trying to generate a sifted frame
            const unsigned int minFramesBeforeVerify = 1;

            /// a mutex for use with collectedStatesCv
            std::mutex statesMutex;
            /// used for waiting for new data to arrive
            std::condition_variable statesCv;
            StatesList collectedStates;
        };

    } // namespace Sift
} // namespace cqp


