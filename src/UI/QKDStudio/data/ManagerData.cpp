/*!
* @file
* @brief ManagerData
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ManagerData.h"

namespace cqp
{
    namespace model
    {

        ManagerData::ManagerData()
        {

        }

        QtNodes::NodeDataType ManagerData::type() const
        {
            return QtNodes::NodeDataType{"ManagerNodeData", "Manager"};
        }
    } // namespace model
} // namespace cqp

