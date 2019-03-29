/*!
* @file
* @brief ErrorCorrection
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <chrono>                                          // for seconds
#include <condition_variable>                              // for condition_...
#include <mutex>                                           // for mutex
#include "CQPToolkit/ErrorCorrection/Stats.h"              // for Stats
#include "CQPToolkit/Interfaces/IErrorCorrectPublisher.h"  // for IErrorCorr...
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"        // for ISiftedCal...
#include "Algorithms/Util/Provider.h"
#include "Algorithms/Util/WorkerThread.h"                  // for WorkerThread
#include "Algorithms/Datatypes/Base.h"                                // for SequenceNu...
#include "QKDInterfaces/IErrorCorrect.grpc.pb.h"           // for IErrorCorrect
#include "CQPToolkit/Interfaces/IRemoteComms.h"


namespace cqp
{
    namespace ec
    {
        /**
         * @brief The ErrorCorrection class
         * Common code for error correction
         */
        class CQPTOOLKIT_EXPORT ErrorCorrection :
        /* Interfaces */
            virtual public ISiftedCallback, virtual public remote::IErrorCorrect::Service,
        /* parents */
            public Provider<IErrorCorrectCallback>, protected WorkerThread,
            public virtual IRemoteComms
        {
        public:

            /**
             * @brief Alignment
             * Constructor
             */
            ErrorCorrection() = default;

            /**
             * @brief ~ErrorCorrection
             * Destructor
             */
            ~ErrorCorrection() override
            {
                Stop(true);
            }

            /**
             * @brief PublishCorrected
             *
             * @startuml PublishCorrectedBehaviour
             * [-> ErrorCorrection : PublishCorrected
             * activate ErrorCorrection
             *  ErrorCorrection -> ErrorCorrection : Emit(correctedData)
             * deactivate ErrorCorrection
             * @enduml
             */
            void PublishCorrected();

            /// @{
            /// @name ISiftCallback Interface

            /**
             * @brief OnSifted
             * @param id
             * @param siftedData
             * @startuml OnSiftedBehaviour
             * [-> ErrorCorrection : OnSifted
             * activate ErrorCorrection
             *  ErrorCorrection -> ErrorCorrection : StoreData
             *  ErrorCorrection -> receivedDataCv : notify_one
             * deactivate ErrorCorrection
             * @enduml
             */
            void OnSifted(const SequenceNumber id, double securityParmeter, std::unique_ptr<JaggedDataBlock> siftedData) override;

            /// @}

            /// @{
            /// @name IRemoteComms interface

            /**
             * @brief Connect
             * @param channel channel to the other side
             */
            void Connect(std::shared_ptr<grpc::ChannelInterface> channel) override;

            /**
             * @brief Disconnect
             */
            void Disconnect() override;

            ///@}

            /// statistics generated by this class
            Stats stats;
        protected:
            /// @{
            /// @name WorkerThread overrides

            /// Perform processing on incoming data
            void DoWork() override;

            /// @}

            /// sequence id for the packet of data passed to the next stage
            SequenceNumber ecSeqId = 0;

        }; // ErrorCorrection
    } // namespace ec
} // namespace cqp


