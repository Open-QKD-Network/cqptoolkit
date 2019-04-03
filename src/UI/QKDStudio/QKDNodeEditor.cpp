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
#include "QKDNodeEditor.h"
#include <nodes/FlowScene>
#include <nodes/FlowView>

#include <nodes/ConnectionStyle>

#include "model/Device.h"
#include "model/Clavis2.h"
#include "model/DummyQKD.h"
#include "model/FreespaceBob.h"
#include "model/Handheld.h"
#include "model/Manager.h"
#include "model/SDN.h"
#include "model/Static.h"
#include "model/SiteAgent.h"


namespace cqp
{
    namespace ui
    {

        void QKDNodeEditor::SetStyle()
        {
            using namespace QtNodes;

            ConnectionStyle::setConnectionStyle(
                R"(
              {
                "ConnectionStyle": {
                  "ConstructionColor": "gray",
                  "NormalColor": "black",
                  "SelectedColor": "gray",
                  "SelectedHaloColor": "deepskyblue",
                  "HoveredColor": "deepskyblue",

                  "LineWidth": 3.0,
                  "ConstructionLineWidth": 2.0,
                  "PointDiameter": 10.0,

                  "UseDataDefinedColors": true
                }
              }
              )");
        }

        std::shared_ptr<QtNodes::DataModelRegistry> QKDNodeEditor::RegisterDataModels()
        {
            auto ret = std::make_shared<QtNodes::DataModelRegistry>();
            const auto Managers = "Managers";
            const auto Sites = "Sites";
            const auto Devices = "Devices";

            ret->registerModel<model::SiteAgent>(Sites);

            ret->registerModel<model::SDN>(Managers);
            ret->registerModel<model::Manager>(Managers);
            ret->registerModel<model::Static>(Managers);

            ret->registerModel<model::Device>(Devices);
            ret->registerModel<model::Clavis2>(Devices);
            ret->registerModel<model::DummyQKD>(Devices);
            ret->registerModel<model::Handheld>(Devices);
            ret->registerModel<model::FreespaceBob>(Devices);


            return ret;
        }

        QKDNodeEditor::QKDNodeEditor() :
            QtNodes::FlowScene(RegisterDataModels())
        {
            using namespace std;
        }

    } // namespace ui
} // namespace cqp
