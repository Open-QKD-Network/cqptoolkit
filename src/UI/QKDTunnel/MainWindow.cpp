/*!
* @file
* @brief MainWindow
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "DeviceDialog.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include <QInputDialog>
#include <QAbstractItemDelegate>
#include <QStyledItemDelegate>
#include <QListView>
#include <QCompleter>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QTextStream>
#include "CQPToolkit/Tunnels/RawSocket.h"
#include "QKDInterfaces/Tunnels.pb.h"
#include "google/protobuf/util/json_util.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "QKDInterfaces/IKeyFactory.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "QKDInterfaces/ITunnelServer.grpc.pb.h"
#include "CQPAlgorithms/Datatypes/UUID.h"

/// cast to int
#define SCtoInt(x) static_cast<int>(x)

namespace qkdtunnel
{
    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow()),
        settingsSaveDialog(this, "Settings file"),
        deviceDialog(new DeviceDialog(this))
    {
        using namespace cqp::tunnels;

        cqp::ConsoleLogger::Enable();
        cqp::DefaultLogger().SetOutputLevel(cqp::LogLevel::Debug);
        ui->setupUi(this);
        servDiscovery.Add(this);

        ui->controllerList->setModel(&controllermodel);

        connect(ui->controllerList->selectionModel(), &QItemSelectionModel::currentRowChanged,
                this, &MainWindow::ControllerSelectionChanged);

        ui->tunOtherController->setModel(&controllermodel);
        ui->keyStoreFactory->setModel(&keyStoresModel);

        connect(ui->keyStoreReferenceGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), this, &MainWindow::KeyStoreByIdClicked);
        connect(ui->controllerReferenceGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), this, &MainWindow::TunOtherControllerByIdClicked);

        settingsSaveDialog.setConfirmOverwrite(true);
        settingsSaveDialog.setDefaultSuffix("json");
        settingsSaveDialog.setMimeTypeFilters({"application/json", "application/octet-stream"});
    } // MainWindow

    MainWindow::~MainWindow()
    {
        delete ui;
    } // ~MainWindow

    void MainWindow::KeyStoreByIdClicked(QAbstractButton*)
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            const bool checked = ui->keyStoreFactoryById->isChecked();
            controller->setData(checked, ControllerItem::Index::localKeyFactoryById);
            if(ui->tunOtherController->currentIndex() < 0)
            {
                // manual data entry
                if(checked)
                {
                    controller->setData("", ControllerItem::Index::localKeyFactoryUri);
                    controller->setData(ui->keyStoreFactory->currentText(), ControllerItem::Index::localKeyFactoryId);
                }
                else
                {
                    controller->setData(ui->keyStoreFactory->currentText(), ControllerItem::Index::localKeyFactoryUri);
                    controller->setData("", ControllerItem::Index::localKeyFactoryId);
                }
            }
        }
    }

    void MainWindow::TunOtherControllerByIdClicked(QAbstractButton*)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            const bool checked = ui->keyStoreFactoryById->isChecked();
            tunnel->setData(checked, TunnelItem::Index::remoteControllerReferenceById);
            if(ui->tunOtherController->currentIndex() < 0)
            {
                // manual data entry
                if(checked)
                {
                    tunnel->setData("", TunnelItem::Index::remoteControllerUri);
                    tunnel->setData(ui->tunOtherController->currentText(), TunnelItem::Index::remoteControllerUuid);
                }
                else
                {
                    tunnel->setData(ui->tunOtherController->currentText(), TunnelItem::Index::remoteControllerUri);
                    tunnel->setData("", TunnelItem::Index::remoteControllerUuid);
                }
            }
        }
    }

    void MainWindow::on_controllerHost_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->controllerHost->text(), ControllerItem::Index::connectionAddress);
        }
    } // on_controllerHost_editingFinished

    void MainWindow::on_controllerName_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->controllerName->text(), ControllerItem::Index::name);
        } // if(controller)
    } // on_controllerName_editingFinished

    void MainWindow::on_controllerDelete_clicked()
    {
        const QModelIndex selected = ui->controllerList->currentIndex();

        controllermodel.removeRow(selected.row(), selected.parent());
        ui->createTunnel->setEnabled(controllermodel.rowCount() >= 1);
    } // on_controllerDelete_clicked

    void MainWindow::on_createTunnel_clicked()
    {
        ControllerItem* parent = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());

        if (parent)
        {
            TunnelItem* child = new TunnelItem(QString::fromStdString("New Tunnel " + std::to_string(uniqueCounter++)));
            const int childRow = parent->rowCount();
            parent->setChild(childRow, 0, child);
            ui->controllerList->setCurrentIndex(controllermodel.index(childRow, 0, parent->index()));
        }
    } // on_createTunnel_clicked

    void MainWindow::on_controllerAdd_clicked()
    {
        controllermodel.appendRow(new ControllerItem(cqp::UUID()));
        const QModelIndex addindex = controllermodel.index(controllermodel.rowCount() - 1, 0);
        ui->controllerList->setCurrentIndex(addindex);

        ui->createTunnel->setEnabled(controllermodel.rowCount() >= 1);
    } // on_controllerAdd_clicked

    void MainWindow::on_manualConnect_clicked()
    {

        QInputDialog dlg;
        dlg.setInputMode(QInputDialog::InputMode::TextInput);
        dlg.setWindowTitle("Connect to controller");
        dlg.setLabelText("Controller address");
        dlg.setInputMethodHints(Qt::InputMethodHint::ImhUrlCharactersOnly);
        dlg.setOkButtonText("Add");
        dlg.exec();
        if(dlg.result() == QInputDialog::Accepted)
        {
            cqp::URI address(dlg.textValue().toStdString());
            cqp::RemoteHost hostDetails;

            hostDetails.host = address.GetHost();
            hostDetails.port = address.GetPort();
            address.GetFirstParameter("id", hostDetails.id);
            if(!address.GetFirstParameter("name", hostDetails.name))
            {
                hostDetails.name = address.GetHostAndPort();
            }
            hostDetails.interfaces.insert(cqp::remote::tunnels::ITunnelServer::service_full_name());

            controllermodel.SetRemote(hostDetails);
        }
    }

    void MainWindow::ControllerSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
    {
        ControllerItem* previousController = controllermodel.FindItem<ControllerItem>(previous);
        TunnelItem* previousTunnel = controllermodel.FindItem<TunnelItem>(previous);

        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(current);
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(current);

        if(controller != nullptr && previousController != controller)
        {
            ui->controllerID->setText(controller->data(ControllerItem::Index::Id).toString());
            ui->controllerName->setText(controller->data(ControllerItem::Index::name).toString());
            ui->controllerHost->setText(controller->data(ControllerItem::Index::connectionAddress).toString());
            ui->listenAddress->setText(controller->data(ControllerItem::Index::listenAddress).toString());
            ui->listenPort->setValue(controller->data(ControllerItem::Index::listenPort).toInt());
            ui->lastUpdated->setText(controller->data(ControllerItem::Index::lastUpdated).toString());

            ui->tunCryptoMode->setModel(controller->GetCryptoModes());
            ui->tunCryptoSubMode->setModel(controller->GetCryptoSubModes());
            ui->tunCryptoBlockCypher->setModel(controller->GetCryptoBlockCyphers());
            ui->tunCryptoKeySize->setModel(controller->GetCryptoKeySizes());

            const bool byId = controller->data(ControllerItem::Index::localKeyFactoryById).toBool();
            if(byId)
            {
                ui->keyStoreFactoryById->setChecked(true);
                ui->keyStoreFactory->setCurrentText(controller->data(ControllerItem::Index::localKeyFactoryId).toString());
            }
            else
            {
                ui->keyStoreFactoryByUri->setChecked(true);
                ui->keyStoreFactory->setCurrentText(controller->data(ControllerItem::Index::localKeyFactoryUri).toString());
            }

            if(controller->data(ControllerItem::Index::credUseTls).toBool())
            {
                ui->certUseTls->setCheckState(Qt::Checked);
            }
            else
            {
                ui->certUseTls->setCheckState(Qt::Unchecked);
            }
            ui->certFile->setText(controller->data(ControllerItem::Index::credCertFile).toString());
            ui->keyFile->setText(controller->data(ControllerItem::Index::credKeyFile).toString());
            ui->caFile->setText(controller->data(ControllerItem::Index::credCaFile).toString());
        }

        // custom linked widgets
        if(tunnel)
        {
            if(previousTunnel != tunnel)
            {
                ui->tunName->setText(tunnel->data(TunnelItem::Index::name).toString());
                ui->tunActivate->setChecked(tunnel->data(TunnelItem::Index::active).toBool());


                ui->tunCryptoBlockCypher->setEditText(tunnel->data(TunnelItem::Index::encryptionMethod_blockCypher).toString());
                ui->tunCryptoKeySize->setEditText(tunnel->data(TunnelItem::Index::encryptionMethod_keySizeBytes).toString());
                ui->tunCryptoMode->setEditText(tunnel->data(TunnelItem::Index::encryptionMethod_mode).toString());
                ui->tunCryptoSubMode->setEditText(tunnel->data(TunnelItem::Index::encryptionMethod_subMode).toString());
                ui->tunCryptoKeySize->setEditText(tunnel->data(TunnelItem::Index::encryptionMethod_keySizeBytes).toString());

                ui->tunKeyMaxBytesScale->setCurrentIndex(tunnel->data(TunnelItem::Index::keyLifespanBytesUnits).toInt());
                ui->tunKeyMaxBytes->setValue(tunnel->data(TunnelItem::Index::keyLifespanBytes).toInt());

                ui->tunKeyMaxTime->setValue(tunnel->data(TunnelItem::Index::keyLifespanAge).toInt());
                ui->tunKeyMaxTimeUnits->setCurrentIndex(tunnel->data(TunnelItem::Index::keyLifespanAgeUnits).toInt());
                ui->tunStartDevice->setText(tunnel->data(TunnelItem::Index::startNode_clientDataPortUri).toString());
                ui->tunEndDevice->setText(tunnel->data(TunnelItem::Index::endNode_clientDataPortUri).toString());

                const bool byId = tunnel->data(TunnelItem::Index::remoteControllerReferenceById).toBool();
                if(byId)
                {
                    ui->tunOtherControllerByID->setChecked(true);
                    ui->tunOtherController->setCurrentText(tunnel->data(TunnelItem::Index::remoteControllerUuid).toString());
                }
                else
                {
                    ui->tunOtherControllerByUri->setChecked(true);
                    ui->tunOtherController->setCurrentText(tunnel->data(TunnelItem::Index::remoteControllerUri).toString());
                }

                if(controller)
                {
                    ui->tunActivate->setEnabled(controller->data(ControllerItem::Index::running).toBool());
                }

                ui->editStack->setCurrentIndex(1);
            }
        }
        else
        {
            ui->editStack->setCurrentIndex(0);
        }

        ui->rightFrame->setEnabled(controller != nullptr);
        ui->tunnelEditPage->setEnabled(tunnel != nullptr);
        ui->controllerDelete->setEnabled(current.isValid());
        ui->controllerHost->setEnabled(controller != nullptr);
        ui->createTunnel->setEnabled(controllermodel.rowCount() >= 1);
    } // ControllerSelectionChanged

    void MainWindow::OnServiceDetected(const cqp::RemoteHosts &newServices, const cqp::RemoteHosts& deletedServices)
    {
        using namespace cqp::remote;
        using namespace cqp::remote::tunnels;

        controllermodel.SetRemote(newServices);
        for(auto& service : deletedServices)
        {
            if(service.second.interfaces.contains(ITunnelServer::service_full_name()))
            {

            }

        }

        for(const auto& newService : newServices)
        {
            if(newService.second.interfaces.contains(IKeyFactory::service_full_name()))
            {
                keyStoresModel.AppendRow(newService.second.name, newService.second.host + ":" + std::to_string(newService.second.port), newService.second.id);

            }

        }
    } // OnServiceDetected

    void MainWindow::on_importSettings_clicked()
    {
        using namespace cqp;
        settingsSaveDialog.setFileMode(QFileDialog::FileMode::ExistingFile);
        settingsSaveDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
        if(settingsSaveDialog.exec() == QDialog::Accepted && !settingsSaveDialog.selectedFiles().empty())
        {
            QFile file(settingsSaveDialog.selectedFiles()[0], this);
            file.open(QIODevice::ReadOnly);
            QTextStream sourceText(&file);

            remote::tunnels::ControllerDetails controllerDetails;

            bool result = LogStatus(google::protobuf::util::JsonStringToMessage(sourceText.readAll().toStdString(), &controllerDetails)).ok();

            if(result)
            {
                ControllerItem* item = controllermodel.GetController(controllerDetails.id());

                if(item)
                {
                    item->SetDetails(controllerDetails);
                }
                else
                {
                    controllermodel.appendRow(new ControllerItem(controllerDetails));
                    // select the new controller
                    const QModelIndex addindex = controllermodel.index(controllermodel.rowCount() - 1, 0);
                    ui->controllerList->setCurrentIndex(addindex);
                    // set button enabled state
                    ui->createTunnel->setEnabled(controllermodel.rowCount() >= 1);
                }
            }

            if(!result)
            {
                QMessageBox errorBox(QMessageBox::Icon::Critical, "Import failed",
                                     "Failed to import settings",
                                     QMessageBox::StandardButton::Ok, this);
                errorBox.exec();
            }
            file.close();
        }

        ui->createTunnel->setEnabled(controllermodel.rowCount() >= 1);
    }

    void MainWindow::on_exportSettings_clicked()
    {
        settingsSaveDialog.setFileMode(QFileDialog::FileMode::AnyFile);
        settingsSaveDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);

        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());

        if(controller)
        {
            settingsSaveDialog.selectFile(controller->GetName());

            if(settingsSaveDialog.exec() == QDialog::Accepted)
            {
                using google::protobuf::util::JsonOptions;
                std::string json;
                JsonOptions options;
                options.add_whitespace = true;
                options.preserve_proto_field_names = true;

                bool result = cqp::LogStatus(google::protobuf::util::MessageToJsonString(controller->GetDetails(), &json, options)).ok();
                if(result)
                {
                    QFile file(settingsSaveDialog.selectedFiles()[0], this);
                    result = file.open(QIODevice::WriteOnly);
                    if(result)
                    {
                        result = file.write(json.data(), static_cast<qint64>(json.size())) == static_cast<qint64>(json.size());
                        file.close();
                    }
                }

                if(!result)
                {
                    QMessageBox errorBox(QMessageBox::Icon::Critical, "Export failed",
                                         "Failed to export settings",
                                         QMessageBox::StandardButton::Ok, this);
                    errorBox.exec();
                }
            }
        }
    }

    void MainWindow::on_tunName_editingFinished()
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(ui->tunName->text(), TunnelItem::Index::name);
        }
    } // on_tunName_editingFinished

    void MainWindow::on_clearModels_clicked()
    {
        controllermodel.clear();
        ui->controllerList->clearSelection();
        ControllerSelectionChanged(QModelIndex(), QModelIndex());

    } //on_clearModels_clicked

    void MainWindow::on_tunActivate_clicked(bool active)
    {
        TunnelItem* siteAItem = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        ControllerItem* siteAController = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());

        if(siteAItem && siteAController)
        {

            //cqp::remote::tunnels::ITunnelServer::NewStub()
            using namespace cqp;
            bool result = false;
            bool retry = false;
            std::string errorMessage;

            do
            {

                cqp::remote::tunnels::Tunnel tunnelSettings = siteAItem->GetDetails();
                auto channel = grpc::CreateChannel(siteAController->GetURI(), grpc::InsecureChannelCredentials());
                if(channel)
                {
                    auto controller = remote::tunnels::ITunnelServer::NewStub(channel);
                    if(controller)
                    {
                        grpc::ClientContext ctx;
                        google::protobuf::Empty response;
                        grpc::Status status;

                        if(!active)
                        {
                            grpc::ClientContext ctx2;
                            google::protobuf::StringValue request;
                            request.set_value(tunnelSettings.name());
                            status = controller->StopTunnel(&ctx2, request, &response);
                        }

                        if(status.ok())
                        {
                            status = LogStatus(controller->ModifyTunnel(&ctx, tunnelSettings, &response));
                        }

                        if(active && status.ok())
                        {
                            grpc::ClientContext ctx2;
                            google::protobuf::StringValue request;
                            request.set_value(tunnelSettings.name());
                            status = LogStatus(controller->StartTunnel(&ctx2, request, &response));
                            result = status.ok();
                        }

                        if(!status.ok())
                        {
                            errorMessage = "Failed to create tunnel:\n" + status.error_details() + "Retry?";
                        }
                    }
                    else
                    {
                        errorMessage = "Failed to connect to controller.\nRetry?";
                    }
                }
                else
                {
                    errorMessage = "Failed to create channel to controller.\nRetry?";
                }

                if(result)
                {
                    ui->tunActivate->setChecked(true);
                }
                else
                {
                    ui->tunActivate->setChecked(false);
                    QMessageBox errorBox(QMessageBox::Icon::Critical, "Tunnel creation failed",
                                         QString::fromStdString(errorMessage),
                                         QMessageBox::StandardButton::Abort | QMessageBox::StandardButton::Retry, this);
                    retry = (errorBox.exec() == QMessageBox::Retry);
                } // if(!result)
            } // do
            while(!result && retry);
        }
    } // on_tunActivate_clicked

    void MainWindow::on_listenPort_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->listenPort->value(), ControllerItem::Index::listenPort);
        }
    }

    void MainWindow::on_tunKeyMaxBytes_valueChanged(int arg1)
    {
        if(arg1 < 0)
        {
            if(ui->tunKeyMaxBytesScale->currentIndex() > 0)
            {
                ui->tunKeyMaxBytes->setValue(1024);
                ui->tunKeyMaxBytesScale->setCurrentIndex(ui->tunKeyMaxBytesScale->currentIndex() - 1);
            }
            else
            {
                // don't let it go below 0
                ui->tunKeyMaxBytes->setValue(0);
            }
        }
        else if(arg1 > 1024)
        {
            if(ui->tunKeyMaxBytesScale->currentIndex() < ui->tunKeyMaxBytesScale->count() - 1)
            {
                ui->tunKeyMaxBytes->setValue(ui->tunKeyMaxBytes->singleStep());
                // move to the next scale
                ui->tunKeyMaxBytesScale->setCurrentIndex(ui->tunKeyMaxBytesScale->currentIndex() + 1);
            }
            else
            {
                // don't let it go above 1024
                ui->tunKeyMaxBytes->setValue(1024);
            }
        }

        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(ui->tunKeyMaxBytes->value(), TunnelItem::Index::keyLifespanBytes);
            tunnel->setData(ui->tunKeyMaxBytesScale->currentIndex(), TunnelItem::Index::keyLifespanBytesUnits);
        }
    }

    void MainWindow::on_tunOtherController_currentIndexChanged(int index)
    {
        ControllerItem* otherController = controllermodel.FindItem<ControllerItem>(controllermodel.index(index, 0));
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(otherController && tunnel)
        {
            tunnel->setData(index, TunnelItem::Index::remoteControllerIndex);
            tunnel->setData(otherController->data(ControllerItem::Index::connectionAddress), TunnelItem::Index::remoteControllerUri);
            tunnel->setData(otherController->data(ControllerItem::Index::Id), TunnelItem::Index::remoteControllerUuid);
        }
    }

    void MainWindow::on_tunCryptoMode_currentTextChanged(const QString &arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(arg1, TunnelItem::Index::encryptionMethod_mode);
        }
    }

    void MainWindow::on_tunCryptoSubMode_currentTextChanged(const QString &arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(arg1, TunnelItem::Index::encryptionMethod_subMode);
        }
    }

    void MainWindow::on_tunCryptoBlockCypher_currentTextChanged(const QString &arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(arg1, TunnelItem::Index::encryptionMethod_blockCypher);
        }
    }

    void MainWindow::on_tunCryptoKeySize_currentTextChanged(const QString &arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(arg1, TunnelItem::Index::encryptionMethod_keySizeBytes);
        }
    }

    void MainWindow::on_tunKeyMaxBytesScale_currentIndexChanged(int index)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(index, TunnelItem::Index::keyLifespanBytesUnits);
        }
    }

    void MainWindow::on_tunKeyMaxTimeUnits_currentIndexChanged(int index)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(index, TunnelItem::Index::keyLifespanAgeUnits);
        }
    }

    void MainWindow::on_tunKeyMaxTime_valueChanged(int arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(arg1, TunnelItem::Index::keyLifespanAge);
        }
    }

    void MainWindow::on_tunStartDeviceEdit_clicked()
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        deviceDialog->SetData(ui->tunStartDevice->text());
        if(tunnel && deviceDialog->exec() == QDialog::Accepted)
        {
            QString finalUrl = deviceDialog->GetUri();
            ui->tunStartDevice->setText(finalUrl);
            tunnel->setData(finalUrl, TunnelItem::Index::startNode_clientDataPortUri);
        }
    }

    void MainWindow::on_tunEndDeviceEdit_clicked()
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        deviceDialog->SetData(ui->tunEndDevice->text());
        if(tunnel && deviceDialog->exec() == QDialog::Accepted)
        {
            QString finalUrl = deviceDialog->GetUri();
            ui->tunEndDevice->setText(finalUrl);
            tunnel->setData(finalUrl, TunnelItem::Index::endNode_clientDataPortUri);
        }
    }

    void MainWindow::on_keyStoreFactory_currentIndexChanged(int index)
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller && index >= 0)
        {
            auto keyStoreIndex = keyStoresModel.index(index, 0);
            controller->setData(index, ControllerItem::Index::localKeyFactoryIndex);
            controller->setData(
                keyStoresModel.data(keyStoreIndex, KeyStoreModel::Index::connectionAddress),
                ControllerItem::Index::localKeyFactoryUri);
            controller->setData(
                keyStoresModel.data(keyStoreIndex, KeyStoreModel::Index::id),
                ControllerItem::Index::localKeyFactoryId);
        }
    }

    void MainWindow::on_tunOtherController_editTextChanged(const QString &arg1)
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(ui->tunOtherController->currentIndex() < 0 && tunnel)
        {
            tunnel->setData(-1, TunnelItem::Index::remoteControllerIndex);
            tunnel->setData(ui->tunOtherControllerByID->isChecked(), TunnelItem::Index::remoteControllerReferenceById);
            tunnel->setData(arg1, TunnelItem::Index::remoteControllerUri);
            tunnel->setData(arg1, TunnelItem::Index::remoteControllerUuid);

        }
    }

    void MainWindow::on_keyStoreFactory_editTextChanged(const QString &arg1)
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(ui->keyStoreFactory->currentIndex() < 0 && controller)
        {
            controller->setData(-1, ControllerItem::Index::localKeyFactoryIndex);
            controller->setData(ui->keyStoreFactoryById->isChecked(), ControllerItem::Index::localKeyFactoryById);
            controller->setData(arg1, ControllerItem::Index::localKeyFactoryUri);
            controller->setData(arg1, ControllerItem::Index::localKeyFactoryId);
        }
    }

    void MainWindow::on_tunEndDevice_editingFinished()
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(ui->tunEndDevice->text(), TunnelItem::Index::endNode_clientDataPortUri);
        }
    }

    void MainWindow::on_tunStartDevice_editingFinished()
    {
        TunnelItem* tunnel = controllermodel.FindItem<TunnelItem>(ui->controllerList->currentIndex());
        if(tunnel)
        {
            tunnel->setData(ui->tunStartDevice->text(), TunnelItem::Index::startNode_clientDataPortUri);
        }
    }

    void MainWindow::on_certFile_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->certFile->text(), ControllerItem::Index::credCertFile);
        }
    }

    void MainWindow::on_keyFile_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->keyFile->text(), ControllerItem::Index::credKeyFile);
        }
    }

    void MainWindow::on_caFile_editingFinished()
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(ui->caFile->text(), ControllerItem::Index::credCaFile);
        }
    }

    void MainWindow::on_certUseTls_stateChanged(int arg1)
    {
        ControllerItem* controller = controllermodel.FindItem<ControllerItem>(ui->controllerList->currentIndex());
        if(controller)
        {
            controller->setData(arg1 == Qt::Checked, ControllerItem::Index::credUseTls);
        }
    }

} // namespace qkdtunnel
