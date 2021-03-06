/*!
* @file
* @brief GatingResponse
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Alignment/Alignment.h"
#include "Algorithms/Random/RandomNumber.h"
#include "CQPToolkit/Interfaces/IEmitterEventPublisher.h"
#include "Algorithms/Util/Provider.h"
#include "QKDInterfaces/IAlignment.grpc.pb.h"
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp
{

    namespace remote
    {
        class DeviceConfig;
    }
    namespace align
    {

        /**
         * @brief The TransmissionHandler class
         * Handles requests for alignment data from the detector
         */
        class CQPTOOLKIT_EXPORT TransmissionHandler : public Alignment,
            public remote::IAlignment::Service,
            public virtual IEmitterEventCallback
        {
        public:
            TransmissionHandler() = default;

            /// @copydoc IEmitterEventCallback::OnEmitterReport
            void OnEmitterReport(std::unique_ptr<EmitterReport> report) override;

            ///@{
            /// @name IAlignment interface

            ///@copydoc remote::IAlignment::GetAlignmentMarkers
            grpc::Status GetAlignmentMarkers(grpc::ServerContext *, const remote::MarkersRequest* request, remote::MarkersResponse *response) override;

            ///@copydoc remote::IAlignment::DiscardTransmissions
            grpc::Status DiscardTransmissions(grpc::ServerContext *, const remote::ValidDetections *request, google::protobuf::Empty *) override;
            ///@}

        protected:
            /// The data to process
            EmitterReportList receivedData;
            /// A source of randomness
            RandomNumber rng;

            /// a mutex for use with receivedDataCv
            std::mutex receivedDataMutex;
            /// used for waiting for new data to arrive
            std::condition_variable receivedDataCv;
        };

    } // namespace align
} // namespace cqp
