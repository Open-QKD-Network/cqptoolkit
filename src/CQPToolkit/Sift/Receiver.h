/*!
* @file
* @brief SiftReceiver
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Sift/SiftBase.h"
#include "QKDInterfaces/ISift.grpc.pb.h"

namespace cqp
{
    namespace sift
    {

        /// Accepts data from the alignment processor and stores it, notifying anything waiting for the data.
        class CQPTOOLKIT_EXPORT Receiver :
                public SiftBase, public remote::ISift::Service
        {
        public:

            /**
             * @brief SiftReceiver
             * Constructor
             */
            Receiver();

            ///@{
            /// @name remote::ISift interface

            /**
             * @copydoc remote::ISift::VerifyBases
             * @param context Connection details from the server
             * @return true on success
             * @details
             * @startuml VerifyBasesBehaviour
             * participant BB84Sifter
             * [-> BB84Sifter : VerifyBases
             * activate BB84Sifter
             *      BB84Sifter -> BB84Sifter : ProcessStates
             * [<-- BB84Sifter : ReturnResults
             * deactivate BB84Sifter
             * @enduml
             */
            grpc::Status VerifyBases(grpc::ServerContext* context, const remote::BasisBySiftFrame* request, remote::AnswersByFrame* response) override;

            ///@}
        protected:
            /// How long to wait for incomming data
            std::chrono::milliseconds receiveTimeout {500};

            /// a mutex for use with collectedStatesCv
            std::mutex statesMutex;
            /// used for waiting for new data to arrive
            std::condition_variable statesCv;

        }; // SiftReceiver

    } // namespace Sift
} // namespace cqp
