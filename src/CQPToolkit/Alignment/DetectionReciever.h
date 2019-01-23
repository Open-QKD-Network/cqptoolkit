/*!
* @file
* @brief DetectionReciever
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Alignment/Alignment.h"
#include "Algorithms/Random/RandomNumber.h"
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include <Algorithms/Util/WorkerThread.h>
#include "Algorithms/Alignment/Filter.h"
#include "Algorithms/Alignment/Gating.h"
#include <grpc++/channel.h>

namespace cqp {
    struct SystemParameters;

namespace align {

    /**
     * @brief The DetectionReciever class
     * handles incoming photon reports
     */
    class CQPTOOLKIT_EXPORT DetectionReciever : public Alignment,
    /* Interfaces */
        virtual public IDetectionEventCallback,
            protected WorkerThread
    {
    public:
        static constexpr SystemParameters DefaultSystemParameters = {
            40000000, // slots per frame
            std::chrono::nanoseconds(100), // slot width
            std::chrono::nanoseconds(1) // jitter
        };
        /// constructor
        DetectionReciever(const SystemParameters &parameters = DefaultSystemParameters);

        /// destructor
        virtual ~DetectionReciever() override = default;

        /// @{
        /// @name IDetectionEventCallback Interface

        /**
         * @brief OnPhotonDetection
         * Photons have been received
         * @param report
         * @startuml OnPhotonBehaviour
         * [-> Alignment : OnTimeTagReport
         * activate Alignment
         *  Alignment -> Alignment : StoreData
         *  Alignment -> receivedDataCv : notify_one
         * deactivate Alignment
         * @enduml
         */
        void OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report) override;

        /// @}

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
            transmitter.reset();
        }

        /**
         * @brief DoWork
         */
        void DoWork() override;

    protected:
        /// storage for incoming data
        ProtocolDetectionReportList receivedData;
        /// Source of randomness
        std::shared_ptr<RandomNumber> rng;
        std::shared_ptr<grpc::Channel> transmitter;
        align::Filter filter;
        align::Gating gating;
    };

} // namespace align
} // namespace cqp
