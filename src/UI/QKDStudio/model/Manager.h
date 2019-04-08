/*!
* @file
* @brief Manager
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

class QFrame;

namespace cqp
{
    namespace model
    {
        class ManagerData;

        /**
         * @brief The Manager class
         * models the network manager
         */
        class Manager : public QtNodes::NodeDataModel
        {
            Q_OBJECT

        public:

            Manager(const std::string& name = "Manager");

            ~Manager() override;

            QString caption() const override
            {
                return QString::fromStdString(managerName);
            }
            QString name() const override
            {
                return QString::fromStdString(managerName);
            }
            unsigned int nPorts(QtNodes::PortType portType) const override;

            QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

            std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

            QWidget* embeddedWidget() override;

            void SetAddress(const std::string& address);
        protected:
            std::shared_ptr<ManagerData> managerData;
            QFrame* container;
            const std::string managerName;

            // NodeDataModel interface
        public:
            bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override;
        };

    } // namespace model
} // namespace cqp


