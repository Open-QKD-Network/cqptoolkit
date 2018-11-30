/*!
* @file
* @brief CQP Toolkit - Key generation
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
#include "CQPAlgorithms/Util/IEvent.h"
#include "CQPAlgorithms/Datatypes/Keys.h"
#include <memory>

namespace cqp
{
    /// Callback class to be implemented when ready to use key is needed.
    class CQPTOOLKIT_EXPORT IKeyCallback
    {
    public:

        /// Called with newly produced key
        /// @param[in] keyData An array of words which represent the key
        virtual void OnKeyGeneration(std::unique_ptr<KeyList> keyData) =0;

        /// pure virtual destructor
        virtual ~IKeyCallback() = default;
    };

}
