/*!
* @file
* @brief IAlignement
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/Framing.h"

namespace cqp
{

    /**
     * @brief The IAlignmentCallback class
     * @details
     * Interface for receiving the results of alignment
     */
    class IAlignmentCallback
    {
    public:

        /**
         * @brief OnAligned
         * @param rawQubits
         * @param seq
         */
        virtual void OnAligned(SequenceNumber seq,
                               std::unique_ptr<QubitList> rawQubits) = 0;

        /// Virtual destructor
        virtual ~IAlignmentCallback() = default;
    };

}
