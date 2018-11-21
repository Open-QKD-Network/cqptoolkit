/*!
* @file
* @brief SiftBase
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/IAlignmentPublisher.h"
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"
#include "CQPToolkit/Util/Provider.h"
#include "CQPToolkit/Sift/Stats.h"
#include "QKDInterfaces/ISift.grpc.pb.h"


namespace cqp
{
    namespace sift
    {

        /**
         * @brief The SiftBase class
         * Common codew for sifting
         */
        class CQPTOOLKIT_EXPORT SiftBase : public IAlignmentCallback, public Provider<ISiftedCallback>
        {
        public:
            /**
             * @brief SiftBase
             * Constructor
             */
            SiftBase() = default;

            ///@{
            /// @name IAlignmentCallback Interface

            /// @copydoc IAlignmentCallback::OnAligned
            void OnAligned(SequenceNumber seq, std::unique_ptr<QubitList> rawQubits) override;

            ///@}

            /// Statistics produced by this class
            Statistics stats;
        protected:
            /**
             * @brief ProcessStates
             * @param start The first element to send, the range must be contiguous
             * @param end The last element to send, the range must be contiguous
             * @param answers The results of the comparison
             * @details
             * @startuml PublishStatesBehaviour
             * [-> BB84Sifter : ProcessStates
             * activate BB84Sifter
             *      BB84Sifter -> BB84Sifter : Emit(validData)
             * deactivate BB84Sifter
             * @enduml
             */
            void PublishStates(QubitsByFrame::iterator start, QubitsByFrame::iterator end, const remote::AnswersByFrame& answers);

            /// a mutex for use with collectedStatesCv
            std::mutex collectedStatesMutex;
            /// used for waiting for new data to arrive
            std::condition_variable collectedStatesCv;
            /// Data that has accumulated
            QubitsByFrame collectedStates;
            /// identifier for this instance
            std::string instance;
            /// Counter for the sequence number used with each publication of a block of qubits
            SequenceNumber siftedSequence = 0;
        };

    } // namespace sift
} // namespace cqp


