/*!
* @file
* @brief SiteAgentCtlGui
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteAgentCtlGui.h"
#include "ui_SiteAgentCtlGui.h"
#include <QInputDialog>
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "QKDInterfaces/Site.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <grpc++/create_channel.h>
#include <google/protobuf/util/json_util.h>
#include <QClipboard>
#include <QMessageBox>
#include "Algorithms/Logging/ConsoleLogger.h"

SiteAgentCtlGui::SiteAgentCtlGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SiteAgentCtlGui)
{
    cqp::ConsoleLogger::Enable();
    cqp::DefaultLogger().SetOutputLevel(cqp::LogLevel::Debug);
    ui->setupUi(this);
}

SiteAgentCtlGui::~SiteAgentCtlGui()
{
    delete ui;
}

cqp::remote::PhysicalPath SiteAgentCtlGui::GetHops()
{
    cqp::remote::PhysicalPath result;
    for(int index = 0 ; index < ui->devicesInHop->topLevelItemCount() ; index++)
    {
        auto item = ui->devicesInHop->topLevelItem(index);

        auto pair = result.mutable_hops()->Add();
        auto first = pair->mutable_first();
        auto second = pair->mutable_second();
        first->set_site(item->text(0).toStdString());
        first->set_deviceid(item->text(1).toStdString());

        second->set_site(item->text(3).toStdString());
        second->set_deviceid(item->text(4).toStdString());

        pair->mutable_params()->set_lineattenuation(item->text(6).toDouble());
    }
    return result;
}

void SiteAgentCtlGui::OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices)
{
    std::lock_guard<std::mutex> lock(localSiteAgentsMutex);
    for(const auto& serv : deletedServices)
    {
        auto foundSites = ui->siteTree->findItems(QString::fromStdString(serv.second.host + ":" + std::to_string(serv.second.port)), Qt::MatchFlag::MatchExactly);
        for(auto found : foundSites)
        {
            ui->siteTree->removeItemWidget(found, 0);
            delete found;
        }
    }

    for(const auto& serv : newServices)
    {
        AddSite(serv.second.host + ":" + std::to_string(serv.second.port));
    }
}

void SiteAgentCtlGui::AddSite(const std::string& address)
{
    using namespace Qt;
    using cqp::remote::ISiteAgent;
    using grpc::ClientContext;
    using google::protobuf::Empty;
    using grpc::Status;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    cqp::remote::Credentials credSettings;
    auto channel = grpc::CreateChannel(address, cqp::LoadChannelCredentials(credSettings));
    if(channel)
    {
        std::unique_ptr<ISiteAgent::Stub> stub =  ISiteAgent::NewStub(channel);

        if(stub)
        {
            ClientContext ctx;
            Empty request;
            cqp::remote::Site siteDetails;
            Status getResult = stub->GetSiteDetails(&ctx, request, &siteDetails);

            if(!getResult.ok())
            {
                QMessageBox::warning(this, tr("Failed to Get site details"), tr(getResult.error_message().c_str()));
            }
            else
            {
                QTreeWidgetItem* existingSite = nullptr;
                auto foundSites = ui->siteTree->findItems(QString::fromStdString(siteDetails.url()), Qt::MatchFlag::MatchExactly);
                if(!foundSites.empty())
                {
                    existingSite = *foundSites.begin();
                }

                if(!existingSite)
                {
                    existingSite = new QTreeWidgetItem(ui->siteTree);
                    existingSite->setText(0, QString::fromStdString(siteDetails.url()));
                }

                for(const auto& device : siteDetails.devices())
                {
                    QTreeWidgetItem* deviceItem = nullptr;
                    for(auto index = 0; index < existingSite->childCount(); index++)
                    {
                        if(existingSite->child(index)->text(0).toStdString() == device.config().id())
                        {
                            deviceItem = existingSite->child(index);
                            break; // for
                        } // if child matches
                    } // for children

                    if(!deviceItem)
                    {
                        deviceItem = new QTreeWidgetItem(existingSite);
                    } // if no device

                    deviceItem->setText(0, QString::fromStdString(device.config().id()));
                    deviceItem->setText(1, QString::fromStdString(device.config().kind()));
                    switch (device.config().side())
                    {
                    case cqp::remote::Side_Type::Side_Type_Alice:
                        deviceItem->setText(2, "Alice");
                        break;
                    case cqp::remote::Side_Type::Side_Type_Bob:
                        deviceItem->setText(2, "Bob");
                        break;
                    case cqp::remote::Side_Type::Side_Type_Any:
                        deviceItem->setText(2, "Any");
                        break;
                    default:
                        break;
                    }
                    deviceItem->setText(3, QString::fromStdString(device.config().switchname()));
                    deviceItem->setText(4, QString::fromStdString(device.config().switchport()));
                } // devices
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Failed to connect"), QString::fromStdString("Failed to connect to " + address));

        }
    }
    QApplication::restoreOverrideCursor();
}

void SiteAgentCtlGui::on_addSite_clicked()
{
    bool ok = false;
    QString address = QInputDialog::getText(this, tr("Add Site Agent"),
                                            tr("Address:"), QLineEdit::Normal,
                                            "", &ok);
    if (ok && !address.isEmpty())
    {
        AddSite(address.toStdString());
    }
}

void SiteAgentCtlGui::on_removeSite_clicked()
{
    for(auto item : ui->siteTree->selectedItems())
    {
        ui->siteTree->removeItemWidget(item, 0);
        delete item;
    }
}

void SiteAgentCtlGui::on_addDeviceFrom_clicked()
{
    for(auto devSelected : ui->siteTree->selectedItems())
    {
        QTreeWidgetItem* newItem = new QTreeWidgetItem(ui->devicesInHop);
        newItem->setText(0, devSelected->parent()->text(0));
        newItem->setText(1, devSelected->text(0));
        newItem->setText(2, devSelected->text(4));
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
    }
}

void SiteAgentCtlGui::on_siteTree_itemClicked(QTreeWidgetItem *item, int)
{
    if(item->columnCount() > 1)
    {
        ui->addDeviceFrom->setEnabled(true);
        ui->addDeviceTo->setEnabled(!ui->devicesInHop->selectedItems().empty());
        ui->devId->setText(item->text(0));
        ui->devKind->setText(item->text(1));
        ui->devSide->setText(item->text(2));
        ui->devSwitchName->setText(item->text(3));
        ui->devSwitchPort->setText(item->text(4));

    }
    else
    {
        ui->addDeviceFrom->setEnabled(false);
        ui->addDeviceTo->setEnabled(false);
        ui->devId->setText("");
        ui->devKind->setText("");
        ui->devSide->setText("");
        ui->devSwitchName->setText("");
        ui->devSwitchPort->setText("");
    }
}

void SiteAgentCtlGui::on_removeDevice_clicked()
{
    for(auto item : ui->devicesInHop->selectedItems())
    {
        ui->devicesInHop->removeItemWidget(item, 0);
        delete item;
    }
}

void SiteAgentCtlGui::on_getJson_clicked()
{
    using google::protobuf::util::JsonOptions;
    JsonOptions options;
    std::string json;
    options.add_whitespace = true;
    cqp::LogStatus(google::protobuf::util::MessageToJsonString(GetHops(), &json, options));
    QApplication::clipboard()->setText(QString::fromStdString(json));
}

void SiteAgentCtlGui::on_devicesInHop_itemClicked(QTreeWidgetItem *, int)
{
    ui->addDeviceTo->setEnabled(!ui->siteTree->selectedItems().empty());
}

void SiteAgentCtlGui::on_addDeviceTo_clicked()
{
    for(auto devSelected : ui->siteTree->selectedItems())
    {
        for(auto item : ui->devicesInHop->selectedItems())
        {
            item->setText(3, devSelected->parent()->text(0));
            item->setText(4, devSelected->text(0));
            item->setText(5, devSelected->text(4));
        }
    }
}

void SiteAgentCtlGui::on_createLink_clicked()
{
    using namespace cqp::remote;
    using grpc::ClientContext;
    using google::protobuf::Empty;
    using grpc::Status;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto hopSpec = GetHops();
    if(hopSpec.hops_size() > 0)
    {
        const auto& firstSite = hopSpec.hops(0).first().site();
        cqp::remote::Credentials credSettings;
        auto channel = grpc::CreateChannel(firstSite, cqp::LoadChannelCredentials(credSettings));
        if(channel)
        {
            std::unique_ptr<ISiteAgent::Stub> stub =  ISiteAgent::NewStub(channel);

            if(stub)
            {
                ClientContext ctx;
                Empty response;
                Status getResult = cqp::LogStatus(stub->StartNode(&ctx, hopSpec, &response));
                if(!getResult.ok())
                {
                    QMessageBox::warning(this, tr("Failed to start node"), tr(getResult.error_message().c_str()));
                }
            }

        }
    }
    QApplication::restoreOverrideCursor();
}

void SiteAgentCtlGui::on_clearHops_clicked()
{
    ui->devicesInHop->clear();
    ui->addDeviceTo->setEnabled(false);
}

void SiteAgentCtlGui::on_clearSites_clicked()
{
    ui->siteTree->clear();
    ui->addDeviceFrom->setEnabled(false);
    ui->addDeviceTo->setEnabled(false);
}

void SiteAgentCtlGui::on_stopLink_clicked()
{
    using namespace cqp::remote;
    using grpc::ClientContext;
    using google::protobuf::Empty;
    using grpc::Status;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto hopSpec = GetHops();
    if(hopSpec.hops_size() > 0)
    {
        const auto& firstSite = hopSpec.hops(0).first().site();
        cqp::remote::Credentials credSettings;
        auto channel = grpc::CreateChannel(firstSite, cqp::LoadChannelCredentials(credSettings));
        if(channel)
        {
            std::unique_ptr<ISiteAgent::Stub> stub =  ISiteAgent::NewStub(channel);

            if(stub)
            {
                ClientContext ctx;
                Empty response;
                Status getResult = cqp::LogStatus(stub->EndKeyExchange(&ctx, hopSpec, &response));
                if(!getResult.ok())
                {
                    QMessageBox::warning(this, tr("Failed to stop node"), tr(getResult.error_message().c_str()));
                }
            }

        }
    }
    QApplication::restoreOverrideCursor();
}
