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
#include "KeyViewer.h"
#include "ui_KeyViewer.h"
#include <grpc++/create_channel.h>
#include <QInputDialog>
#include <QMessageBox>
#include "QKDInterfaces/IKey.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Util/ConsoleLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <openssl/pem.h>
#include <openssl/pem2.h>
#include <openssl/pkcs12.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/KeyGen/YubiHSM.h"

using grpc::Status;
using grpc::StatusCode;
using google::protobuf::Empty;
using grpc::ClientContext;
using cqp::remote::IKey;

KeyViewer::KeyViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KeyViewer),
    pinDialog(this)
{
    cqp::ConsoleLogger::Enable();
    cqp::DefaultLogger().SetOutputLevel(cqp::LogLevel::Debug);
    sd.Add(this);
    ui->setupUi(this);
    ui->sendToHSM->setMenu(&hsmMenu);

    connect(&hsmMenu, &QMenu::aboutToShow, this, &KeyViewer::OnSendToHSMShow);
    connect(&hsmMenu, &QMenu::aboutToHide, this, &KeyViewer::OnSendToHSMHide);
    connect(&hsmMenu, &QMenu::triggered, this, &KeyViewer::HSMPicked);
}

KeyViewer::~KeyViewer()
{
    sd.Remove(this);
    delete ui;
}

void KeyViewer::OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices)
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

void KeyViewer::ClearKey()
{
    keyData.clear();
    ui->keyValue->clear();
    ui->keyId->clear();
    keyId = 0;
    ui->keyActionsPage->setEnabled(false);
    ui->keyStack->setCurrentIndex(0);
}

void KeyViewer::on_knownSites_currentRowChanged(int currentRow)
{
    bool enabled = currentRow >= 0;
    ui->getNewKey->setEnabled(enabled);
    ui->getExistingKey->setEnabled(enabled);
}

void KeyViewer::on_getNewKey_clicked()
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
            keyData.clear();
            keyData.append(response.keyvalue().data(), response.keyvalue().size());
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

void KeyViewer::on_getExistingKey_clicked()
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

void KeyViewer::on_revealKey_clicked()
{
    ui->keyStack->setCurrentIndex(1);
}

void KeyViewer::OnSendToHSMShow()
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

        QAction* hsmButton = new QAction(QString::fromStdString(dictionary["token"]), this);
        hsmButton->setProperty("url", QString::fromStdString(token));
        hsmButton->setProperty("module-name", QString::fromStdString(dictionary["module-name"]));
        hsmMenu.addAction(hsmButton);
    }
    QApplication::restoreOverrideCursor();
}

void KeyViewer::OnSendToHSMHide()
{

}

void KeyViewer::HSMPicked(QAction* action)
{
    using cqp::keygen::YubiHSM;
    using cqp::keygen::HSMStore;
    using cqp::keygen::IBackingStore;

    std::unique_ptr<HSMStore> theStore;
    const std::string hsmUrl = action->property("url").toString().toStdString();
    if(action->property("module-name").toString().contains("yubi"))
    {
        theStore.reset(new YubiHSM(hsmUrl, &pinDialog));

    } else {
        theStore.reset(new HSMStore(hsmUrl, &pinDialog));
    }

    theStore->InitSession();

    IBackingStore::Keys keys;
    keys.push_back({keyId, {keyData.begin(), keyData.end()}});
    if(theStore->StoreKeys(ui->knownSites->currentItem()->text().toStdString(), keys))
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

void KeyViewer::on_clearKey_clicked()
{
    ClearKey();
}

void KeyViewer::on_localAgentGo_clicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    ui->knownSites->clear();
    ui->knownSites->setEnabled(false);
    cqp::remote::Credentials credSettings;

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


void KeyViewer::on_addModule_clicked()
{
    const auto newMod = QInputDialog::getText(this, "Add Module","Module to add:").toStdString();
    if(!newMod.empty())
    {
        knownModules.push_back(newMod);
    }
}
