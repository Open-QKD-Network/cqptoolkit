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
#include "Device.h"
#include "data/LinkData.h"
#include <QLayout>
#include <QToolButton>
#include <QScrollArea>
#include "data/SiteAgentData.h"
#include "DeviceEditor.h"
#include <QAction>

namespace cqp
{
    namespace model
    {

        Device::Device(const std::string& name, remote::Side_Type side):
            driverName{name},
            deviceEditor{std::make_unique<DeviceEditor>()}
        {
            details.mutable_config()->set_side(side);

            topWidget = new QScrollArea();
            topWidget->setWidgetResizable(true);
            QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            auto layout = new QHBoxLayout(topWidget);
            topWidget->setLayout(layout);
            topWidget->setSizePolicy(sizePolicy);
            topWidget->resize(0,0);

            auto connectBtn = new QToolButton(topWidget);
            connectBtn->setIcon(QIcon::fromTheme(QString::fromUtf8("network-connect")));
            layout->addWidget(connectBtn);

            auto disconnect = new QToolButton(topWidget);
            disconnect->setIcon(QIcon::fromTheme(QString::fromUtf8("network-disconnect")));
            disconnect->setEnabled(false);
            layout->addWidget(disconnect);

            auto edit = new QToolButton(topWidget);
            edit->setIcon(QIcon::fromTheme(QString::fromUtf8("edit")));
            layout->addWidget(edit);

            QObject::connect(edit, &QToolButton::clicked, this, &Device::OnEdit);
            QObject::connect(deviceEditor.get(), &DeviceEditor::finished, this, &Device::OnEditFinished);
        }

        Device::~Device()
        {

        }

        QString Device::caption() const
        {
            auto sideStr = "Alice";
            switch (details.config().side())
            {
            case remote::Side_Type::Side_Type_Bob:
                sideStr = "Bob";
                break;
            default:
                break;
            }
            return QString::fromStdString(driverName + " : " + sideStr);
        }

        unsigned int Device::nPorts(QtNodes::PortType portType) const
        {
            unsigned int result = 1;

            switch (portType)
            {
            case QtNodes::PortType::In:
                result = 1; // to site agent

                if(details.config().side() == remote::Side_Type::Side_Type_Alice)
                {
                    result++;
                }
                break;

            case QtNodes::PortType::Out:
                result = 0; // device to device?
                if(details.config().side() == remote::Side_Type::Side_Type_Bob)
                {
                    result++;
                }
                break;

            default:
                break;
            }

            return result;
        }

        QtNodes::NodeDataType Device::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QtNodes::NodeDataType result;

            switch (portType)
            {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
                case 0:
                    result = SiteAgentData().type();
                    break;
                case 1:
                    result = LinkData().type();
                    break;
                }
                break;

            case QtNodes::PortType::Out:
                switch (portIndex)
                {
                case 0:
                    result = LinkData().type();
                    break;
                }
                break;
            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        QString Device::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QString result;
            switch (portType)
            {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
                case 0:
                    result = QStringLiteral("Site");
                    break;
                case 1:
                    result = QStringLiteral("Link");
                }
                break;

            case QtNodes::PortType::Out:
                switch (portIndex)
                {
                case 0:
                    result = QStringLiteral("Link");
                    break;
                }
                break;
            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        void Device::StartSession(std::shared_ptr<LinkData> newLink)
        {
            if(linkData)
            {
                // TODO
            }

        }

        void Device::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port)
        {
            switch (port)
            {
            case 0:
                // register with site agent
                details.set_siteagentaddress(std::dynamic_pointer_cast<SiteAgentData>(nodeData)->address);
                break;
            case 1:
                // connect to device
                StartSession(std::dynamic_pointer_cast<LinkData>(nodeData));

                break;
            default:
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> Device::outData(QtNodes::PortIndex port)
        {
            switch (port)
            {
            case 0:
            {
                if(!linkData)
                {
                    linkData = std::make_shared<LinkData>();
                }
                return linkData;
            }
            }

            return nullptr;
        }

        QWidget* Device::embeddedWidget()
        {
            return topWidget;
        }

        void Device::SetDetails(const remote::ControlDetails& details)
        {
            this->details = details;
        }

        bool Device::captionVisible() const
        {
            return true;
        }

        bool Device::portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const
        {
            return true;
        }

        QtNodes::NodeDataModel::ConnectionPolicy Device::portOutConnectionPolicy(QtNodes::PortIndex port) const
        {
            using QtNodes::NodeDataModel;
            NodeDataModel::ConnectionPolicy result = NodeDataModel::ConnectionPolicy::Many;

            switch (port)
            {
            case 0:
                result = NodeDataModel::ConnectionPolicy::One;
                break;
            }

            return result;
        }

        void Device::OnEdit()
        {
            deviceEditor->SetDetails(details);
            deviceEditor->open();
        }

        void Device::OnEditFinished(int result)
        {
            if(result == QDialog::Accepted)
            {
                deviceEditor->UpdateDetails(details);
            }
        }
    } // namespace model
} // namespace cqp


