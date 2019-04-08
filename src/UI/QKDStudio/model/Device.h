/*!
* @file
* @brief Device
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <nodes/NodeDataModel>
#include <QObject>
#include "QKDInterfaces/Device.pb.h"

class QScrollArea;

namespace cqp
{
    class DeviceEditor;

    namespace model
    {
        class LinkData;

        /**
         * @brief The Device class
         * models a device
         */
        class Device : public QtNodes::NodeDataModel
        {
            Q_OBJECT

        public:

            /**
             * @brief Device
             * @param name
             * @param side
             */
            Device(const std::string& name = "Device", remote::Side_Type side = remote::Side_Type::Side_Type_Alice);

            /// distructor
            ~Device() override;

            QString caption() const override;
            QString name() const override
            {
                return QString::fromStdString(driverName);
            }
            unsigned int nPorts(QtNodes::PortType portType) const override;

            QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

            std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

            QWidget* embeddedWidget() override;

            void SetDetails(const remote::ControlDetails& details);

            void StartSession(std::shared_ptr<LinkData> newLink);
        protected:
            std::shared_ptr<LinkData> linkData;
            const std::string driverName;

            remote::ControlDetails details;
            QScrollArea* topWidget;

            std::unique_ptr<DeviceEditor> deviceEditor;
            // NodeDataModel interface
        public:
            bool captionVisible() const override;
            bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override;

            // NodeDataModel interface
        public:
            ConnectionPolicy portOutConnectionPolicy(QtNodes::PortIndex) const override;

        protected slots:
            void OnEdit();
            void OnEditFinished(int result);
        };

    } // namespace model
} // namespace cqp


