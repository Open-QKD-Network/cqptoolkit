/*!
* @file
* @brief QKDNodeEditor
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <memory>
#include <nodes/FlowScene>

namespace QtNodes
{
    class FlowScene;
    class DataModelRegistry;
}

namespace cqp
{
    namespace ui
    {

        /**
         * @brief The QKDNodeEditor class provides a graphical editor for sites and devices
         * it uses the [node editor](https://github.com/paceholder/nodeeditor) from @cite Pinaev2017
         */
        class QKDNodeEditor : public QtNodes::FlowScene
        {
        public:
            QKDNodeEditor();

            /**
             * @brief SetStyle
             */
            static void SetStyle();
        protected: // methods

            /**
             * @brief RegisterDataModels
             * @return
             */
            static std::shared_ptr<QtNodes::DataModelRegistry> RegisterDataModels();

        protected: // members
        };

    } // namespace ui
} // namespace cqp


