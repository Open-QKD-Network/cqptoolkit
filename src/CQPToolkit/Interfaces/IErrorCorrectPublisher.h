/*!
* @file
* @brief IErrorCorrect
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 26/6/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Base.h"
#include <memory>

namespace cqp
{

    /**
     * @brief The IErrorCorrectCB class
     */
    class IErrorCorrectCallback
    {
    public:

        /// identifier for data which has been processed in one block
        using ValidatedBlockID = SequenceNumber;

        /**
         * @brief OnCorrected
         * @param blockId The block which has been checked
         * @param correctedData Error free data
         */
        virtual void OnCorrected(
            const ValidatedBlockID blockId,
            std::unique_ptr<DataBlock> correctedData) = 0;

        /// destructor
        virtual ~IErrorCorrectCallback() = default;
    };

}
