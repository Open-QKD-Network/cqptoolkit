/*!
* @file
* @brief SiteAgent
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteAgent.h"
#include <QLineEdit>
#include "data/SiteAgentData.h"
#include "data/ManagerData.h"
#include <QLayout>
#include <QToolButton>
#include <QScrollArea>
#include <QInputDialog>
#include <grpcpp/create_channel.h>
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "SiteEditor.h"
#include "KeyViewer.h"
#include <QLabel>
#include "Algorithms/Datatypes/URI.h"

namespace cqp
{
    namespace model
    {

        SiteAgent::SiteAgent() :
            siteData{std::make_shared<SiteAgentData>()},
            siteEditor{std::make_unique<SiteEditor>()},
            keyViewer{std::make_unique<KeyViewer>()}
        {

            topWidget = new QScrollArea();
            topWidget->setWidgetResizable(true);
            QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            auto layout = new QHBoxLayout(topWidget);
            topWidget->setLayout(layout);
            topWidget->setSizePolicy(sizePolicy);
            topWidget->resize(0,0);

            auto getKey = new QToolButton(topWidget);
            getKey->setIcon(QIcon(tr(":/icons/keys")));
            layout->addWidget(getKey);

            auto connect = new QToolButton(topWidget);
            connect->setIcon(QIcon::fromTheme(QString::fromUtf8("network-connect")));
            layout->addWidget(connect);

            disconnect = new QToolButton(topWidget);
            disconnect->setIcon(QIcon::fromTheme(QString::fromUtf8("network-disconnect")));
            disconnect->setEnabled(false);
            layout->addWidget(disconnect);

            auto edit = new QToolButton(topWidget);
            edit->setIcon(QIcon::fromTheme(QString::fromUtf8("edit")));
            layout->addWidget(edit);

            QObject::connect(connect, &QToolButton::clicked, this, &SiteAgent::OnConnect);
            QObject::connect(disconnect, &QToolButton::clicked, this, &SiteAgent::OnDisconnect);
            QObject::connect(edit, &QToolButton::clicked, this, &SiteAgent::OnEdit);
            QObject::connect(siteEditor.get(), &SiteEditor::finished, this, &SiteAgent::OnEditFinished);
            QObject::connect(getKey, &QToolButton::clicked, this, &SiteAgent::GetKey);

            creds = grpc::InsecureChannelCredentials();
        }

        SiteAgent::~SiteAgent()
        {

        }

        QString SiteAgent::caption() const
        {
            QString result;

            if(!config.name().empty())
            {
                result = QString::fromStdString(config.name());
            }
            else if(!details.url().empty())
            {
                result =QString::fromStdString(details.url());
            }
            else
            {
                result = QStringLiteral("Site Agent");
            }

            return result;
        }

        unsigned int SiteAgent::nPorts(QtNodes::PortType portType) const
        {
            unsigned int result = 1;

            switch (portType)
            {
            case QtNodes::PortType::In:
                result = 1 + bobDevices.size(); // to netman
                break;

            case QtNodes::PortType::Out:
                result = aliceDevices.size(); // from devices
                break;

            default:
                break;
            }

            return result;
        }

        QtNodes::NodeDataType SiteAgent::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QtNodes::NodeDataType result;

            switch (portType)
            {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
                case 0:
                    result = ManagerData().type();
                    break;
                default:
                    if(portIndex <= bobDevices.size())
                    {
                        result = LinkData().type();
                    }
                }
                break;

            case QtNodes::PortType::Out:

                if(portIndex >= 0 && portIndex < aliceDevices.size())
                {
                    result = LinkData().type();
                }
                break;

            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        QString SiteAgent::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
        {
            QString result;
            switch (portType)
            {
            case QtNodes::PortType::In:
                if(portIndex == 0)
                {
                    result = "Manager";
                }
                else if(portIndex > 0 && portIndex <= bobDevices.size())
                {
                    result = QString::fromStdString(
                                 bobDevices[portIndex - 1].kind());
                }
                break;

            case QtNodes::PortType::Out:
                if(portIndex >= 0 && portIndex < aliceDevices.size())
                {
                    result = QString::fromStdString(
                                 aliceDevices[portIndex].kind());
                }

                break;

            case QtNodes::PortType::None:
                break;
            }

            return result;
        }

        void SiteAgent::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port)
        {
            switch (port)
            {
            case 0:
            {
                if(nodeData)
                {
                    // register with network manager
                    auto managerData = std::dynamic_pointer_cast<ManagerData>(nodeData);
                    config.set_netmanuri(managerData->address);
                }
                else
                {
                    config.set_netmanuri("");
                }
            }
            break;
            default:
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> SiteAgent::outData(QtNodes::PortIndex port)
        {
            std::shared_ptr<QtNodes::NodeData> result;
            switch (port)
            {
            case 0:
            {
                // connection address for device
                result = siteData;
            }
            break;
            default:
                break;
            }
            return result;
        }

        QWidget*SiteAgent::embeddedWidget()
        {
            return topWidget;
        }

        void SiteAgent::SetDetails(const remote::Site& details)
        {
            this->details = details;
            siteData->address = details.url();
        }

        void SiteAgent::SetAddress(const std::string& address)
        {
            details.set_url(address);
            siteData->address = details.url();
        }

        void SiteAgent::OnConnect()
        {
            bool ok = false;
            details.set_url(QInputDialog::getText(nullptr,
                                                  "Site Agent Address",
                                                  "Host and Port", QLineEdit::Normal,
                                                  QString::fromStdString(details.url()), &ok).toStdString());

            siteData->address = details.url();

            if(ok)
            {
                Connect();
            }
        }

        void SiteAgent::Connect()
        {
            channel = grpc::CreateChannel(details.url(), creds);

            auto stub = remote::ISiteAgent::NewStub(channel);
            using google::protobuf::Empty;
            grpc::ClientContext ctx;

            if(LogStatus(stub->GetSiteDetails(&ctx, Empty(), &details)).ok())
            {
                disconnect->setEnabled(true);

                for(auto dev : details.devices())
                {
                    LOGINFO("Adding device called: " + dev.config().kind());
                    if(dev.config().side() == remote::Side::Alice)
                    {
                        aliceDevices.emplace_back(dev.config());
                    }
                    else
                    {
                        bobDevices.emplace_back(dev.config());
                    }
                }
            }
        }

        void SiteAgent::OnDisconnect()
        {
            channel.reset();
            disconnect->setEnabled(false);
        }

        void SiteAgent::OnEdit()
        {
            siteEditor->SetConfig(config);
            siteEditor->open();
        }

        void SiteAgent::OnEditFinished(int result)
        {
            if(result == QDialog::Accepted)
            {
                siteEditor->UpdateSite(config);
                // TODO: trigger redraw
            }
        }

        void SiteAgent::GetKey()
        {
            if(!details.url().empty())
            {
                keyViewer->SetSourceSite(details.url());
            }
            keyViewer->open();
        }

        bool SiteAgent::portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const
        {
            return true;
        }
    } // namespace model
} // namespace cqp

