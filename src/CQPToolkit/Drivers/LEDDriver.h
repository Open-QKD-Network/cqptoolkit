/*!
* @file
* @brief CQP Toolkit - LED Driver board
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Drivers/Serial.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Datatypes/Qubits.h"

//////// DANGER : In Flux ///////
namespace cqp
{
    /// Driver for Daves LED driver board
    class CQPTOOLKIT_EXPORT LEDDriver : public Serial
    {

    protected:
        // Instruct the compiler that we do not want any extra bytes to be added inbetween values in the structures
        // Care should be taken to ensure that each structure meets the alignment requirements of the target system.
#pragma pack(push, 1)

        /// The command being issued or replyed to.
        enum class Command : uint8_t
        {
            Invalid     = 0x00,
            Test        = 0x08,
            Data        = 0x07
        };

        /// Data associated with the Test command
        struct Test
        {
        public:
            /// The kind of test pattern
            enum class Action : uint8_t
            {
                All_Ones        = 0x11,
                All_Twos        = 0x12,
                All_Threes      = 0x13,
                All_Fours       = 0x14,
                Increment       = 0x17,
                Decrement       = 0x18,
                Stop            = 0x1F
            };
            /// The kind of test pattern
            Action action;
        };

        /// Data associated with the Data command.
        struct Data
        {
        public:
            /// First element in the array of data
            Qubit    bits;
        };

        /// Data preoended to all packets sent to/received from the device
        struct Header
        {
        public:
            /// The command to be performed
            /// This will effect the length of the message
            Command     cmd;
            /// The crc of the entire message
            /// @todo Does this include the header or not.
            uint8_t     crc;
            /// The length of the entire message
            uint32_t    msgLength;
        };

        /// Definition of all possible structures which could appear after the header.
        /// The valid element of this union is defined by the cmd value in the header.
        union Payload
        {
            /// ?
            Test    test;
            /// Data associated with the Data command.
            Data    data;
        };

        /// The definition of a complete message in including header.
        struct Message
        {
            /// preamble for message
            Header      header;
            /// specific message data
            Payload     payload;
        };
        /// Exit the pack mode and return the packing to it's previous value.
#pragma pack(pop)

    };
}
