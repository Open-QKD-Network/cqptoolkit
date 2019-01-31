/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/5/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/algorithms_export.h"
#include <string>
#include <vector>
#include "Algorithms/Datatypes/Base.h"
#include <unordered_map>
#include <memory>

namespace cqp
{

    /// The name used to identify Qubit classes
    static const std::string QubitName = "Qubit";

    /// Definition of a single photon
    using Qubit = uint8_t;

    /// Defines the possible orientation of the Qubit
    enum class Basis : uint8_t
    {
        Retiliniear = 0x00,
        Diagonal    = 0x02,
        Circular    = 0x04,    // For compatibility with future systems
        Invalid     = 0x06,

        _First = Retiliniear,
        _Last = Invalid
    }; // Basis

    /// A collection of basis
    using BasisList = std::vector<Basis>;

    /// Encoding scheme which includes the Basis and the bit value of a Qubit
    enum class BB84 : uint8_t
    {
        Zero    = 0x00,
        One     = 0x01,
        Pos     = 0x02,
        Neg     = 0x03,
        Right   = 0x04,
        Left    = 0x05,
        Invalid = 0x06,

        _First = Zero,
        _Last = Invalid
    }; // BB84

    /// Definition and accessibility operators for a single Qubit
    /// @details Each qubit takes 2 bits to store the information.
    /// There are a number of different schemes for representing a qubit
    /// Currently only BB84 is provided.
    class ALGORITHMS_EXPORT QubitHelper
    {
    public:

        /// Returns the Basis of the Qubit, discarding the binary value
        /// @param storedValue input value
        /// @returns the basis of the qubit
        static Basis Base(Qubit storedValue)
        {
            return static_cast<Basis>(storedValue & 0x06);
        } // Base

        /// The binary value of the Qubit, discarding the basis.
        /// @param storedValue input value
        /// @return the binary value of the qubit
        static bool BitValue(Qubit storedValue)
        {
            return storedValue & 0x01;
        } // BitValue

    }; // Qubit

    /// A list of Qubits
    /// @todo override the vector functions to improve storage performance
    using QubitList = std::vector<Qubit>;

    /// A dictionary of QubitLists indexed by SequenceNumber
    using QubitsByFrame = std::unordered_map<SequenceNumber, std::unique_ptr<QubitList>>;

    /// Identifier type for slots
    using SlotID = uint64_t;

    using QubitsBySlot = std::unordered_map<SlotID, Qubit>;

}
