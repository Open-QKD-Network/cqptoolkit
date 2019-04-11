/*!
* @file
* @brief QKDStudio
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "QKDStudio.h"
#include "ui_QKDStudio.h"
#include "QKDNodeEditor.h"
#include <QMessageBox>
#include <QGraphicsItem>
#include <nodes/internal/NodeGraphicsObject.hpp>
#include <nodes/internal/Node.hpp>

#include <QInputDialog>
#include <grpcpp/create_channel.h>
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "QKDInterfaces/INetworkManager.grpc.pb.h"
#include "QKDInterfaces/IDevice.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "ConnectDialog.h"

#include "model/SiteAgent.h"
#include "model/Device.h"
#include "model/Manager.h"

#include "Algorithms/Logging/ConsoleLogger.h"
#include "KeyViewer.h"

namespace cqp
{
    namespace ui
    {
        QKDStudio::QKDStudio(QWidget* parent, Qt::WindowFlags flags) :
            QMainWindow(parent, flags)
        {
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Debug);

            creds = grpc::InsecureChannelCredentials();

            ui = std::make_unique<Ui::QKDStudio>();
            ui->setupUi(this);
            connectDialog = std::make_unique<ConnectDialog>(this);

            QKDNodeEditor::SetStyle();

            nodeData = std::make_unique<QKDNodeEditor>();

            ui->nodeWidget->setScene(nodeData.get());

            connect(nodeData.get(), &QKDNodeEditor::selectionChanged,
                    this, &QKDStudio::OnSelectionChanged);

            connect(ui->actionConnect, &QAction::triggered, this, &QKDStudio::OnConnectTo);

            connect(connectDialog.get(), &ConnectDialog::finished, this, &QKDStudio::GrpcConnectionFinished);
            connect(ui->actionZoom_In, &QAction::triggered, ui->nodeWidget, &QtNodes::FlowView::scaleUp);
            connect(ui->actionZoom_Out, &QAction::triggered, ui->nodeWidget, &QtNodes::FlowView::scaleDown);

            connect(nodeData.get(), &QtNodes::FlowScene::connectionCreated, this, &QKDStudio::ConnectionCreated);
            connect(ui->actionDelete_All, &QAction::triggered, nodeData.get(), &QtNodes::FlowScene::clearScene);
            connect(ui->actionDelete, &QAction::triggered, ui->nodeWidget, &QtNodes::FlowView::deleteSelectedNodes);
            connect(ui->actionKey_Viewer, &QAction::triggered, this, &QKDStudio::ShowKeyViewer);

        }

        QKDStudio::~QKDStudio()
        {

        }

        void QKDStudio::OnSelectionChanged()
        {
            if(!nodeData->selectedItems().empty())
            {
                auto selected = nodeData->selectedItems().first();
                auto asNode = dynamic_cast<QtNodes::NodeGraphicsObject*>(selected);

                if(asNode)
                {
                    auto dataModel = asNode->node().nodeDataModel();
                    dataModel->name();
                }
            }
        }

        bool QKDStudio::AddLiveSiteAgent(const std::string& address)
        {
            using namespace std;
            using google::protobuf::Empty;

            bool result = false;
            grpc::ClientContext ctx;
            remote::Site siteDetails;

            auto channel = grpc::CreateChannel(address, creds);
            auto stub = remote::ISiteAgent::NewStub(channel);

            if(LogStatus(stub->GetSiteDetails(&ctx, Empty(), &siteDetails)).ok())
            {
                result = true;
                // this is a site agent
                auto siteModel = std::make_unique<model::SiteAgent>();
                siteModel->SetDetails(siteDetails);
                siteModel->SetAddress(address);
                auto& siteNode = nodeData->createNode(std::move(siteModel));

                for(const auto& device : siteDetails.devices())
                {
                    auto newDevice = std::make_unique<model::Device>();
                    newDevice->SetDetails(device);
                    auto& devNode = nodeData->createNode(std::move(newDevice));

                    nodeData->createConnection(devNode, 0, siteNode, 0);
                }
            }
            return result;
        }

        bool QKDStudio::AddLiveManager(const std::string& address)
        {
            using google::protobuf::Empty;

            bool result = true;
            using namespace std;
            using google::protobuf::Empty;

            grpc::ClientContext ctx;
            remote::SiteDetailsList registered;

            auto channel = grpc::CreateChannel(address, creds);
            auto stub = remote::INetworkManager::NewStub(channel);

            if(LogStatus(stub->GetRegisteredSites(&ctx, Empty(), &registered)).ok())
            {
                result = true;
                // this is a manager
                auto managerModel = std::make_unique<model::Manager>();
                managerModel->SetAddress(address);
                auto& managerNode = nodeData->createNode(std::move(managerModel));

                for(const auto& site : registered.sites())
                {
                    auto newSite = std::make_unique<model::SiteAgent>();
                    newSite->SetDetails(site);
                    auto& siteNode = nodeData->createNode(std::move(newSite));

                    nodeData->createConnection(siteNode, 0, managerNode, 0);

                    for(const auto& device : site.devices())
                    {
                        auto newDevice = std::make_unique<model::Device>();
                        newDevice->SetDetails(device);
                        auto& devNode = nodeData->createNode(std::move(newDevice));

                        nodeData->createConnection(devNode, 0, siteNode, 0);
                    }
                }
            }
            return result;
        }

        bool QKDStudio::AddLiveDevice(const std::string& address)
        {
            using google::protobuf::Empty;

            bool result = false;
            grpc::ClientContext ctx;
            remote::ControlDetails deviceDetails;

            auto channel = grpc::CreateChannel(address, creds);
            auto stub = remote::IDevice::NewStub(channel);

            if(LogStatus(stub->GetDetails(&ctx, Empty(), &deviceDetails)).ok())
            {
                result = true;
                // this is a device
                auto newDevice = std::make_unique<model::Device>();
                newDevice->SetDetails(deviceDetails);
                nodeData->createNode(std::move(newDevice));

            }
            return result;
        }

        void QKDStudio::OnConnectTo()
        {
            connectDialog->open();
        }

        void QKDStudio::GrpcConnectionFinished(int result)
        {
            if(result == QDialog::Accepted)
            {

                switch (connectDialog->GetType())
                {
                case ConnectDialog::ConnectionType::Site:
                    AddLiveSiteAgent(connectDialog->GetAddress());
                    break;
                case ConnectDialog::ConnectionType::Device:
                    AddLiveDevice(connectDialog->GetAddress());
                    break;
                case ConnectDialog::ConnectionType::Manager:
                    AddLiveManager(connectDialog->GetAddress());
                    break;
                }
            }
        }

        void QKDStudio::ConnectionCreated(const QtNodes::Connection& conn)
        {
            using namespace QtNodes;
            auto inNode = conn.getNode(PortType::In);
            auto outNode = conn.getNode(PortType::Out);

            if(inNode && outNode)
            {

            }
        }

        void QKDStudio::ShowKeyViewer()
        {
            KeyViewer viewer;
            viewer.exec();
        }
    }
}
