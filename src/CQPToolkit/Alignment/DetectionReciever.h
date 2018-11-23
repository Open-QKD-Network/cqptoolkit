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
#include "CQPToolkit/Alignment/DetectionGating.h"
#include "CQPToolkit/Simulation/RandomNumber.h"
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"

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
        /// constructor
        DetectionReciever();

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
         * @brief SetSystemParameters
         * @param parameters Values to set
         * @return true on success
         */
        bool SetSystemParameters(const SystemParameters& parameters);

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
        RandomNumber rng;
        DetectionGating gatingHandler;
        std::shared_ptr<grpc::Channel> transmitter;
    };

} // namespace align
} // namespace cqp
