/*!
* @file
* @brief PrivacyAmplify
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/IErrorCorrectPublisher.h"
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "Algorithms/Util/Provider.h"
#include "Algorithms/Util/WorkerThread.h"
#include "QKDInterfaces/IPrivacyAmplify.grpc.pb.h"
#include "CQPToolkit/PrivacyAmp/Stats.h"

namespace cqp
{
    namespace privacy
    {
        /**
         * @brief The PrivacyAmplify class
         * A simple example of aligning time tags as they are received.
         */
        class CQPTOOLKIT_EXPORT PrivacyAmplify :
        /* Interfaces */
            virtual public IErrorCorrectCallback, public remote::IPrivacyAmplify::Service,
        /* parents */
            public Provider<IKeyCallback>, public WorkerThread
        {
        public:

            /**
             * @brief PrivacyAmplify
             * Constructor
             */
            PrivacyAmplify() = default;

            /**
             * @brief PerformPrivacyAmplify
             *
             * @startuml PerformPrivacyAmplifyBehaviour
             * [-> PrivacyAmplify : PublishPrivacyAmplify
             * activate PrivacyAmplify
             *  PrivacyAmplify -> PrivacyAmplify : Emit(alignedData)
             * deactivate PrivacyAmplify
             * @enduml
             */
            void PublishPrivacyAmplify();

            ~PrivacyAmplify() override {
                Stop(true);
            }
            /// the publisher for this instance
            Statistics stats;

        protected:
            /// @{
            /// @name WorkerThread overrides

            /// Perform processing on incoming data
            void DoWork() override;

            /// @}

            /// data received from error correction
            std::vector<std::unique_ptr<DataBlock>> incommingData;
            // IErrorCorrectCallback interface
        public:
            void OnCorrected(const ValidatedBlockID blockId, std::unique_ptr<DataBlock> correctedData) override;
        }; // PrivacyAmplify
    } // namespace privacy
} // namespace cqp


