/*!
* @file
* @brief Privacy datatypes
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Framing.h"
#include "Algorithms/Datatypes/Qubits.h"

namespace cqp
{
    /**
     * @brief The ReferenceData struct
     * A slice of data
     */
    struct ALGORITHMS_EXPORT ReferenceData
    {
        /// The block from which the data was taken
        SequenceNumber blockId;
        ///  The number of bytes from the start of the block which the data starts
        const unsigned long offset;
        /// Selected bytes provided to perform privacy amplification
        DataBlock values;
    };

    /// A list of ReferenceData
    using ReferenceDataList = std::vector<ReferenceData>;

}
