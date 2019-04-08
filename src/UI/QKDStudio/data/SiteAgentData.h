/*!
* @file
* @brief SiteAgentData
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
#include <memory>
#include "QKDInterfaces/Site.pb.h"

namespace cqp
{
    namespace model
    {
        class ManagerData;

        /**
         * @brief The SiteAgentData class
         */
        class SiteAgentData : public QtNodes::NodeData
        {
        public:
            /// constructor
            SiteAgentData();

            // NodeData interface
        public:
            QtNodes::NodeDataType type() const override;
            std::string address;
        };

    } // namespace model
} // namespace cqp


