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
#include <chrono>
#include <grpc++/channel.h>
#include "CQPToolkit/Interfaces/IRemoteComms.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "QKDInterfaces/ISift.grpc.pb.h"

namespace cqp
{
    namespace sift
    {

        /**
         * @brief The BB84Sifter class
         * Sends incoming qubits to the verifier
         * @details
         */
        class CQPTOOLKIT_EXPORT Transmitter :
                protected WorkerThread, public SiftBase,
                public virtual IRemoteComms
        {
        public:
            /**
             * @brief BB84Sifter
             * Constructor
             * @param framesBeforeVerify How many frames to collect before verifying data.
             */
            explicit Transmitter(unsigned int framesBeforeVerify = 1);

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

            /**
             * @brief ~BB84Sifter
             * Destructor
             */
            ~Transmitter() override;

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
            /// The other side to communicate with during sifting
            std::unique_ptr<remote::ISift::Stub> verifier;
            /// How long to wait for new data before checking if the thread should be stopped
            const std::chrono::seconds threadTimeout {1};
            /// How many aligned frames to receive before trying to generate a sifted frame
            const unsigned int minFramesBeforeVerify = 1;

        };

    } // namespace Sift
} // namespace cqp


