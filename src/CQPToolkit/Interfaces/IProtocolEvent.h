/*!
* @file
* @brief CQP Toolkit - Protocol event interface
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13 April 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Algorithms/Datatypes/Framing.h"
#include "Algorithms/Datatypes/Qubits.h"
#pragma once

namespace cqp
{
    /// Takes raw key data and performs error correction on it
    /// Calls IErrorCorrectionCallback::OnCorrectedKey with error free data.
    class CQPTOOLKIT_EXPORT IProtocolEvent
    {
    public:

        /// Notifies the other side that the we no longer want to exchange key
        /// and has completed all steps in the exchange with the result in parameters
        /// @param[in] session The the session to close
        virtual void CloseSession(
            const SessionID& session) = 0;

        /// pure virtual destructor
        virtual ~IProtocolEvent() = default;

        /// The frame id used at the start of each session
        const SequenceNumber firstFrameID = 1;
    };
}
