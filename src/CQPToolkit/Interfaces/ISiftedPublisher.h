/*!
* @file
* @brief %{Cpp:License:ClassName}
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
     * @brief The ISiftCallback class
     */
    class ISiftedCallback
    {
    public:
        /**
         * @brief OnSifted
         * Called by the ISiftPublisher when sifted data is available
         * @param id The sifted frame id for the data
         * @param siftedData The data which has been matched to the other side.
         */
        virtual void OnSifted(
            const SequenceNumber id,
            std::unique_ptr<JaggedDataBlock> siftedData) = 0;

        virtual ~ISiftedCallback() = default;
    };

}
