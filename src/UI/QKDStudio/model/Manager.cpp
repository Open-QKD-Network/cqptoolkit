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
#include "Manager.h"
#include <QLineEdit>
#include "data/ManagerData.h"
#include <QPixmap>
#include <QGridLayout>
#include <QLabel>

namespace cqp
{
    namespace model
    {

        Manager::Manager(const std::string& name):
            managerData{std::make_shared<ManagerData>()},
            container{new QFrame()},
                  managerName{name}
        {
            auto label = new QLabel(container);
            auto managerIcon = QPixmap(":/icons/manager");
            managerIcon.scaledToWidth(32);
            label->setPixmap(managerIcon);
            label->resize(32, 32);
            label->setScaledContents(true);
        }

        Manager::~Manager()
        {

        }

        unsigned int Manager::nPorts(QtNodes::PortType portType) const
        {
            unsigned int result = 1;

            switch (portType)
            {
            case QtNodes::PortType::In:
                result = 0; // QPA?
                break;

            case QtNodes::PortType::Out:
                result = 1; // sites
                break;

            default:
                break;
            }

            return result;
        }

        QtNodes::NodeDataType Manager::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QtNodes::NodeDataType result;

            switch (portType)
            {
            case QtNodes::PortType::In:
                break;

            case QtNodes::PortType::Out:
                switch (portIndex)
                {
                case 0:
                    result = ManagerData().type();
                }
                break;

            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        QString Manager::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QString result;
            switch (portType)
            {
            case QtNodes::PortType::In:
                break;
            case QtNodes::PortType::Out:
                switch (portIndex)
                {
                case 0:
                    result = QStringLiteral("Sites");
                }
                break;

            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        void Manager::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port)
        {
            switch (port)
            {
            case 0:
                break;
            default:
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> Manager::outData(QtNodes::PortIndex port)
        {
            return managerData;
        }

        QWidget*Manager::embeddedWidget()
        {
            return container;
        }

        void Manager::SetAddress(const std::string& address)
        {
            managerData->address = address;
        }

        bool Manager::portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const
        {
            return true;
        }
    } // namespace model
} // namespace cqp

