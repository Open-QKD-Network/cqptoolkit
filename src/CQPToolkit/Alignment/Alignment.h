/*!
* @file
* @brief Alignment
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Alignment/Stats.h"                   // for Statistics
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"    // for IAlignmentC...
#include "Algorithms/Util/Provider.h"                        // for Event, Even...
#include "CQPToolkit/cqptoolkit_export.h"
#include "Algorithms/Datatypes/Qubits.h"

namespace cqp
{
    namespace remote
    {
        class QubitsByFrame;
    }

    namespace align
    {
        /**
         * @brief The Alignment class
         * A simple example of aligning time tags as they are received.
         */
        class CQPTOOLKIT_EXPORT Alignment : public Provider<ISiftedCallback>
        {
        public:

            /// Statistics collected by this class
            Statistics stats;
        protected:

            /**
             * @brief Alignment
             * Constructor
             */
            Alignment();

            void SendResults(const QubitList& emissions, double securityParameter);

            /// our alignment sequence counter
            SequenceNumber seq = 0;

        }; // Alignment

    } // namespace align
} // namespace cqp


