/*!
* @file
* @brief DummyTimeTagger
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"
#include "Algorithms/Util/Provider.h"
#include <chrono>
#include "QKDInterfaces/IDetector.grpc.pb.h"
#include "QKDInterfaces/IPhotonSim.grpc.pb.h"
#include "CQPToolkit/Statistics/Frames.h"
#include "CQPToolkit/Interfaces/IRemoteComms.h"

namespace cqp
{
    class IRandom;

    namespace sim
    {
        /**
         * @brief The DummyTimeTagger class
         * Provide a fake time tagger which gets it's "qubits"
         * from the DummyTransmitter
         */
        class CQPTOOLKIT_EXPORT DummyTimeTagger :
            public remote::IDetector::Service,
            public remote::IPhotonSim::Service,
            public Provider<IDetectionEventCallback>,
            public virtual IRemoteComms
        {
        public:
            /**
             * @brief DummyTimeTagger
             * Constructor
             * @param randomSource Random number generator
             */
            explicit DummyTimeTagger(IRandom* randomSource);

            ///@{
            /// @name remote::IPhotonSim interface

            /// @copydoc remote::IPhotonSim::OnPhoton
            /// @param context Connection details from the server
            grpc::Status OnPhoton(grpc::ServerContext *context, const remote::FakeDetection *request, google::protobuf::Empty *) override;

            ///@}
            ///@{
            /// @name remote::IDetector interface

            /// @copydoc remote::IDetector::StartDetecting
            /// @param context Connection details from the server
            grpc::Status StartDetecting(grpc::ServerContext* context, const google::protobuf::Timestamp* request, google::protobuf::Empty*) override;
            /// @copydoc remote::IDetector::StopDetecting
            /// @param context Connection details from the server
            grpc::Status StopDetecting(grpc::ServerContext* context, const google::protobuf::Timestamp* request, google::protobuf::Empty*) override;
            ///@}

            ///@{
            /// @name IRemoteComms interface

            /**
             * @brief Connect
             * @param channel The IPhotonSim endpoint to send photons to
             */
            void Connect(std::shared_ptr<grpc::ChannelInterface> channel) override;

            /**
             * @brief Disconnect
             */
            void Disconnect() override;

            ///@}

            /// Statistics produced by this class
            stats::Frames stats;
        protected:
            /// Protection for collectedPhotons
            std::mutex collectedPhotonsMutex;
            /// The photons which have arrived
            DetectionReportList collectedPhotons;
            /// The point at which the frame was started
            std::chrono::high_resolution_clock::time_point epoc;
            /// current frame number
            SequenceNumber frame = 1;
            /// random number source
            IRandom* rng = nullptr;

        }; // class DummyTimeTagger
    } // namespace sim
} // namespace cqp


