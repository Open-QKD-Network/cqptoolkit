/*!
* @file
* @brief CQP Toolkit - Key Receiver
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
#include "CQPAlgorithms/Datatypes/Framing.h"
#include "CQPAlgorithms/Datatypes/Qubits.h"
#include "CQPToolkit/Interfaces/IProtocolEvent.h"

namespace cqp
{
    /**
     * @brief The IKeyReceiver class
     * @details External interface which will provide a quantum receiver
     */
    class CQPTOOLKIT_EXPORT IKeyReceiver : public virtual IProtocolEvent
    {
    public:
        /// Causes the Key receiver to begin session negotiation and ultimately transfer key data
        virtual void BeginSession() = 0;

        /// Notifies the receiver of the transmitters configuration and possible protocols
        virtual void ReturnCapabilities() =0;

        /// Notifies the receiver that the transmitter has begun a session and is waiting for
        /// ReadyToReceive()
        /// @param[in] session The id for the session which is being started
        virtual void InitSession(
            const SessionID& session) =0;

        /// Call this to notify the receive that the transmitter has finished sending a block
        /// @note Once ready, the receiver should call ReadyToReceive()
        /// @param[in] session The id for the session which this frame relates to
        /// @param[in] frame The frame which is complete
        virtual void FrameComplete(
            const SessionID& session,
            const SequenceNumber& frame) =0;

        /// pure virtual destructor
        virtual ~IKeyReceiver() = default;
    };
}
