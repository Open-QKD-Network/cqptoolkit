/*!
* @file
* @brief MainWindow
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <grpc++/create_channel.h>
#include <QInputDialog>
#include <QMessageBox>
#include "QKDInterfaces/IKey.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Util/ConsoleLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "HSMPinDialog.h"
#include <openssl/pem.h>
#include <openssl/pem2.h>
#include <openssl/pkcs12.h>
#include "CQPToolkit/Util/GrpcLogger.h"

using grpc::Status;
using grpc::StatusCode;
using google::protobuf::Empty;
using grpc::ClientContext;
using cqp::remote::IKey;

namespace keyviewer
{

    const std::vector<std::string> knownModules = {"libsofthsm2.so"};

    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
    {
        cqp::ConsoleLogger::Enable();
        cqp::DefaultLogger().SetOutputLevel(cqp::LogLevel::Debug);
        sd.Add(this);
        ui->setupUi(this);
        ui->sendToHSM->setMenu(&hsmMenu);

        connect(&hsmMenu, &QMenu::aboutToShow, this, &MainWindow::OnSendToHSMShow);
        connect(&hsmMenu, &QMenu::aboutToHide, this, &MainWindow::OnSendToHSMHide);
        connect(&hsmMenu, &QMenu::triggered, this, &MainWindow::HSMPicked);
    }

    MainWindow::~MainWindow()
    {
        sd.Remove(this);
        delete ui;
    }

    void MainWindow::OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices)
    {
        std::lock_guard<std::mutex> lock(localSiteAgentsMutex);
        for(auto serv : deletedServices)
        {
            for(int itemindex = 0; itemindex < ui->localSiteAgents->count(); itemindex++)
            {
                if(ui->localSiteAgents->itemData(itemindex, Qt::EditRole).toString().toStdString() ==
                        serv.second.host + ":" + std::to_string(serv.second.port))
                {
                    ui->localSiteAgents->removeItem(itemindex);
                    break; // for
                }
            }
        }

        for(auto serv : newServices)
        {
            bool found = false;
            for(int itemindex = 0; itemindex < ui->localSiteAgents->count(); itemindex++)
            {
                if(ui->localSiteAgents->itemData(itemindex, Qt::EditRole).toString().toStdString() ==
                        serv.second.host + ":" + std::to_string(serv.second.port))
                {
                    found = true;
                    break; // for
                }
            }

            if(!found && serv.second.interfaces.contains(cqp::remote::IKey::service_full_name()))
            {
                ui->localSiteAgents->addItem(QString::fromStdString(serv.second.host + ":" + std::to_string(serv.second.port)));
            }
        }
    }

    void MainWindow::ClearKey()
    {
        keyData.clear();
        ui->keyValue->clear();
        ui->keyId->clear();
        keyId = 0;
        ui->keyActionsPage->setEnabled(false);
        ui->keyStack->setCurrentIndex(0);
    }

    void MainWindow::on_knownSites_currentRowChanged(int currentRow)
    {
        bool enabled = currentRow >= 0;
        ui->getNewKey->setEnabled(enabled);
        ui->getExistingKey->setEnabled(enabled);
    }

    void MainWindow::on_getNewKey_clicked()
    {
        std::shared_ptr<IKey::Stub> stub = IKey::NewStub(channel);
        if(stub)
        {
            ClientContext ctx;
            cqp::remote::KeyRequest request;
            cqp::remote::SharedKey response;
            request.set_siteto(ui->knownSites->currentItem()->text().toStdString());

            Status getResult = stub->GetSharedKey(&ctx, request, &response);
            if(getResult.ok())
            {
                ui->keyId->setText(QString::fromStdString(std::to_string(response.keyid())));
                QByteArray array(response.keyvalue().data(), response.keyvalue().size());
                ui->keyValue->setPlainText(QString(array.toHex()));
                ui->keyActionsPage->setEnabled(true);
            }
            else
            {
                QMessageBox successDialog(QMessageBox::Icon::Critical,
                                          QString::fromStdString("Get New Key"),
                                          QString::fromStdString("Failed to get key:\n" + cqp::StatusToString(getResult) + ": " + getResult.error_message()),
                                          QMessageBox::Ok, this);
                successDialog.exec();
                ClearKey();
                ui->keyActionsPage->setEnabled(false);
            }
        }
    }

    void MainWindow::on_getExistingKey_clicked()
    {
        ui->keyId->clear();
        ui->keyValue->clear();
        ui->keyStack->setCurrentIndex(0);

        QInputDialog keyIdDialog(this);
        keyIdDialog.setInputMode(QInputDialog::InputMode::IntInput);
        keyIdDialog.setIntMinimum(0);
        keyIdDialog.setLabelText("Please enter the key ID");
        keyIdDialog.setWindowTitle("Enter Key ID");
        if(keyIdDialog.exec() == QDialog::Accepted)
        {
            std::shared_ptr<IKey::Stub> stub = IKey::NewStub(channel);
            if(stub)
            {
                ClientContext ctx;
                cqp::remote::KeyRequest request;
                cqp::remote::SharedKey response;
                request.set_siteto(ui->knownSites->currentItem()->text().toStdString());
                request.set_keyid(keyIdDialog.intValue());

                Status getResult = stub->GetSharedKey(&ctx, request, &response);
                if(getResult.ok())
                {
                    keyId = response.keyid();
                    ui->keyId->setText(QString::fromStdString(std::to_string(response.keyid())));
                    keyData.clear();
                    keyData.append(response.keyvalue().data(), response.keyvalue().size());
                    ui->keyValue->setPlainText(QString(keyData.toHex()));
                    ui->keyActionsPage->setEnabled(true);
                }
                else
                {
                    ui->keyValue->setPlainText(QString::fromStdString(cqp::StatusToString(getResult) + ": " + getResult.error_message()));
                    ui->keyActionsPage->setEnabled(false);
                }
            }
        }
    }

    void MainWindow::on_revealKey_clicked()
    {
        ui->keyStack->setCurrentIndex(1);
    }

    void MainWindow::OnSendToHSMShow()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        for(auto act : hsmMenu.actions())
        {
            delete act;
        }
        hsmMenu.clear();

        // build a list of HSMs
        auto foundTokens = cqp::keygen::HSMStore::FindTokens(knownModules);
        for(auto token : foundTokens)
        {
            std::map<std::string, std::string> dictionary;
            token.ToDictionary(dictionary);

            QAction* hsmButton = new QAction(QString::fromStdString(token["token"]), this);
            hsmButton->setProperty("url", QString::fromStdString(token));
            hsmMenu.addAction(hsmButton);
        }
        QApplication::restoreOverrideCursor();
    }

    void MainWindow::OnSendToHSMHide()
    {

    }

    void MainWindow::HSMPicked(QAction* action)
    {
        using cqp::keygen::HSMStore;
        using cqp::keygen::IBackingStore;

        HSMStore theStore(
            action->property("url").toString().toStdString(),
            this
        );

        IBackingStore::Keys keys;
        keys.push_back({keyId, {keyData.begin(), keyData.end()}});
        if(theStore.StoreKeys(ui->knownSites->currentItem()->text().toStdString(), keys))
        {

            QMessageBox successDialog(QMessageBox::Icon::Information, "Key Transfer", "Key transfered successfully", QMessageBox::Ok, this);
            successDialog.exec();
            ClearKey();
        }
        else
        {

            QMessageBox successDialog(QMessageBox::Icon::Critical, "Key Transfer", "Key transfered Failed.", QMessageBox::Ok, this);
            successDialog.exec();
        }
    }

    void MainWindow::on_clearKey_clicked()
    {
        ClearKey();
    }

    bool MainWindow::GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel, cqp::keygen::UserType& login, std::string& pin)
    {
        bool result = false;
        // make a dialog to ask for pin
        HSMPinDialog dialog(this, QString::fromStdString(tokenLabel + "(" + tokenSerial + ")"));
        if(dialog.exec() == QDialog::Accepted)
        {
            login = dialog.GetUserType();
            pin = dialog.GetPassword().toStdString();
            result = true;
        }

        return result;
    }

    void MainWindow::on_localAgentGo_clicked()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        ui->knownSites->clear();
        ui->knownSites->setEnabled(false);
        cqp::remote::Credentials credSettings;
        credSettings.set_usetls(ui->useTls->checkState() == Qt::Checked);
        credSettings.set_rootcertsfile(ui->certCa->text().toStdString());

        channel = grpc::CreateChannel(ui->localSiteAgents->currentText().toStdString(), cqp::LoadChannelCredentials(credSettings));
        if(channel)
        {
            std::unique_ptr<cqp::remote::IKey::Stub> stub =  IKey::NewStub(channel);

            if(stub)
            {
                ClientContext ctx;
                Empty request;
                cqp::remote::SiteList sites;
                Status getResult = stub->GetKeyStores(&ctx, request, &sites);

                if(getResult.ok())
                {
                    for(auto site : sites.urls())
                    {
                        ui->knownSites->addItem(QString::fromStdString(site));
                    }

                    ui->knownSites->setEnabled(true);
                }
            }
        }

        QApplication::restoreOverrideCursor();
    }

} // namespace keyviewer

