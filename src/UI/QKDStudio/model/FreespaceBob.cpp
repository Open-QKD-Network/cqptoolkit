/*!
* @file
* @brief FreespaceBob
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "FreespaceBob.h"

namespace cqp
{
    namespace model
    {

        FreespaceBob::FreespaceBob() :
            Device("Freespace Bob", remote::Side_Type::Side_Type_Bob)
        {

        }

    } // namespace model
} // namespace cqp

