/*!
* @file
* @brief CQP Toolkit - Ramdom Number Generation
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/algorithms_export.h"
#include "Algorithms/Datatypes/Qubits.h"

namespace cqp
{
    /// Provides a standard source of random numbers.
    /// @todo Make the interface useful
    class ALGORITHMS_EXPORT IRandom
    {
    public:

        /// Get a random unsigned integer
        /// @return Random bits
        virtual unsigned long RandULong() =0;

        /**
         * @brief RandomBytes
         * Generate a block of random bytes
         * @param numOfBytes The number of byte to make
         * @param dest The storage for the bytes
         */
        virtual void RandomBytes(size_t numOfBytes, DataBlock& dest) = 0;

        /**
         * @brief RandQubitList
         * @param NumQubits The number of qubits to return
         * @return A random selection of valid Qubits
         */
        virtual QubitList RandQubitList(size_t numQubits) = 0;

        /// pure virtual destructor
        virtual ~IRandom() = default;
    };
}
