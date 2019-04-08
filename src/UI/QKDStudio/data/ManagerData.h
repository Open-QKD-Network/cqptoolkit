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
#pragma once
#include <nodes/NodeData>


namespace cqp
{
    namespace model
    {

        /**
         * @brief The ManagerData class
         * Models the network manager
         */
        class ManagerData : public QtNodes::NodeData
        {
        public:
            ManagerData();

            std::string address;
            // NodeData interface
        public:
            QtNodes::NodeDataType type() const override;
        };

    } // namespace model
} // namespace cqp


