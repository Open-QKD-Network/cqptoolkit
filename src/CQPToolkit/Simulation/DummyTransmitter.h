/*!
* @file
* @brief DummyTransmitter
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Provider.h"
#include "Algorithms/Random/IRandom.h"
#include "CQPToolkit/Interfaces/IPhotonGenerator.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "QKDInterfaces/IPhotonSim.grpc.pb.h"
#include "CQPToolkit/Simulation/Stats.h"

namespace cqp
{
    namespace sim
    {

        /**
         * @brief The DummyTransmitter class
         * Provide a fake transmitter which sends it's "qubits"
         * to the DummyTimeTagger
         *
         */
        class CQPTOOLKIT_EXPORT DummyTransmitter : public virtual IPhotonGenerator,
        // publish the details of the photons generated
            public Provider<IEmitterEventCallback>
        {
        public:
            /// Statistics produced by this class
            Statistics stats; // struct Statistics

            /**
             * @brief DummyTransmitter
             * Constructor
             * @param randomSource The source of randomness for generating qubits
             * @param transmissionDelay Time between each qubit transmission
             * @param photonsPerBurst How many photons to send each time Fire() is called
             */
            DummyTransmitter(IRandom* randomSource,
                             PicoSeconds transmissionDelay = std::chrono::nanoseconds(100),
                             size_t photonsPerBurst = 100000);

            ~DummyTransmitter() override;

            /**
             * @brief Connect
             * @param channel The IPhotonSim endpoint to send photons to
             */
            void Connect(std::shared_ptr<grpc::ChannelInterface> channel);

            /**
             * @brief Disconnect
             */
            void Disconnect();

            ///@{
            /// @name IPhotonGenerator Interface

            /// @copydoc IPhotonGenerator::Fire
            void Fire() override;

            /// @copydoc IPhotonGenerator::StartFrame
            void StartFrame() override;

            /// @copydoc IPhotonGenerator::EndFrame
            void EndFrame() override;

            ///@}

        protected:

            /// The other side to communicate with during sifting
            std::unique_ptr<remote::IPhotonSim::Stub> detector;
            /// The point at which the frame was started
            std::chrono::high_resolution_clock::time_point epoc;
            /// current frame number
            SequenceNumber frame = 1;
            /// Delay between each photon transmission
            PicoSeconds txDelay;
            /// Source of randomness for generating qubits
            IRandom* randomness = nullptr;
            /// how many photons to send in one go
            unsigned long photonsPerBurst;
        }; // DummyTransmitter

    } // namespace sim
} // namespace cqp


