/*!
* @file
* @brief LinkData
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include <nodes/NodeData>

namespace cqp
{
    namespace model
    {

        /**
         * @brief The LinkData class holds data for a link between qkd devices
         */
        class LinkData : public QtNodes::NodeData
        {
        public:
            /**
             * @brief LinkData constructor
             */
            LinkData();
            /**
             * @brief type
             * @return Data type for noteeditor glue
             */
            QtNodes::NodeDataType type() const override;
        };

    } // namespace model
} // namespace cqp


