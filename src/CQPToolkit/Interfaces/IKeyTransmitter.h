/*!
* @file
* @brief CQP Toolkit - Key transmission
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "CQPToolkit/cqptoolkit_export.h"
#include "Algorithms/Datatypes/Framing.h"
#include "Algorithms/Datatypes/Qubits.h"
#include "CQPToolkit/Interfaces/IProtocolEvent.h"

namespace cqp
{
    /// External interface which will provide a quantum source
    class CQPTOOLKIT_EXPORT IKeyTransmitter : public virtual IProtocolEvent
    {
    public:
        /// The receiver would like to initiate an exchange
        /// @details This is the first step in the protocol, the transmitter will respond
        /// by calling ReturnCapabilities()
        virtual void RequestCapabilities() =0;

        /// Notifies the transmitter that the receiver is ready to accept QBits
        /// @param[in] session The id for the session which this frame relates to
        /// @param[in] frame The frame which is about to be received
        virtual void ReadyToReceive(
            const SessionID& session,
            const SequenceNumber& frame) =0;

        /// The receiver will call this if it has agreed on a protocol.
        virtual void RequestKeySession() =0;

        /// pure virtual destructor
        virtual ~IKeyTransmitter() = default;
    };
}
