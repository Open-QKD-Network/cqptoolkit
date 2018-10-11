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
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Datatypes/Qubits.h"

namespace cqp
{
    /// Provides a standard source of random numbers.
    /// @todo Make the interface usful
    class CQPTOOLKIT_EXPORT IRandom
    {
    public:

        /// Get a random unsigned integer
        /// @return Random bits
        virtual unsigned long RandULong() =0;

        /// Get a random Qubit
        /// @return A random Qubit
        virtual Qubit RandQubit() = 0;

        /// pure virtual distructor
        virtual ~IRandom() = default;

        // TODO: virtual void Fill(IntList& out, size_t count) = 0;
    };
}
