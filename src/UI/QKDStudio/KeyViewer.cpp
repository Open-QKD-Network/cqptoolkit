/*!
* @file
* @brief cqp::KeyViewer
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 10/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyViewer.h"
#include "ui_KeyViewer.h"
#include "QKDInterfaces/IKey.grpc.pb.h"
#include <grpcpp/create_channel.h>
#include "CQPToolkit/Util/GrpcLogger.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QBitmap>
#include <QLinearGradient>
#include <QPainter>
#include <QFileDialog>
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Datatypes/URI.h"
// qr stuff
#include <qrencode.h>

using google::protobuf::Empty;
using grpc::Status;
using grpc::StatusCode;

namespace cqp
{

    KeyViewer::KeyViewer(QWidget *parent, std::shared_ptr<grpc::ChannelCredentials> credentials) :
        QDialog(parent),
        ui(std::make_unique<Ui::KeyViewer>()),
        rekeyTimer(this)
    {
        ui->setupUi(this);

        // add the radio button to the radio group
        ui->formatGroup->setId(ui->formatHex, FormatGroupIds::hex);
        ui->formatGroup->setId(ui->formatBase64, FormatGroupIds::base64);

        // setup the timer for repeated key generation
        rekeyTimer.setInterval(std::chrono::seconds(ui->refreshTime->value()));
        rekeyTimer.setTimerType(Qt::TimerType::CoarseTimer);
        rekeyTimer.setSingleShot(false);

        connect(&rekeyTimer, &QTimer::timeout, this, &KeyViewer::OnKeyRefresh);

        // setup default creds
        creds = credentials;
        if(!creds)
        {
            creds = grpc::InsecureChannelCredentials();
        }
    }

    KeyViewer::~KeyViewer()
    {
    }

    void KeyViewer::SetSourceSite(const std::string& siteFrom)
    {
        ui->fromSite->setText(QString::fromStdString(siteFrom));
        siteFromChannel = grpc::CreateChannel(siteFrom, creds);
        ui->toSite->clear();
        // populate the list of possible destinations
        ui->toSite->addItems(GetDestinations());
    }

    QStringList KeyViewer::GetDestinations()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QStringList result;
        auto stub = remote::IKey::NewStub(siteFromChannel);
        if(stub)
        {
            grpc::ClientContext ctx;
            remote::SiteList sites;
            // get the list of key stores from the source
            auto getResult = stub->GetKeyStores(&ctx, Empty(), &sites);
            if(LogStatus(getResult).ok())
            {
                // build the display list
                for(const auto& remoteSite : sites.urls())
                {
                    result.push_back(QString::fromStdString(remoteSite));
                } // for sites
            }
            else
            {
                QMessageBox::critical(this, QString::fromStdString("Failed to connect"),
                                      QString::fromStdString(getResult.error_message()));
            }
        } // if stub

        QApplication::restoreOverrideCursor();
        return result;
    }

    void KeyViewer::DisplayKey(const remote::SharedKey& key)
    {
        // set the key id
        ui->id->setText(QString::fromStdString(std::to_string(key.keyid())));
        // for converting the key data for display
        auto rawKey = QByteArray(key.keyvalue().data(), static_cast<int>(key.keyvalue().size()));

        switch (ui->formatGroup->checkedId())
        {
        case FormatGroupIds::hex:
            ui->keyHexView->setPlainText(rawKey.toHex());
            break;
        case FormatGroupIds::base64:
            ui->keyHexView->setPlainText(rawKey.toBase64());
            break;
        }

        // generate the qr code
        qrCodeImage = GenerateQrCode(KeyToJSON(ui->fromSite->text().toStdString(),
                                               ui->toSite->currentItem()->text().toStdString(), key));
        // scale it for the window
        ui->qrCodeView->setPixmap(qrCodeImage.scaled(ui->qrCodeView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->saveQr->setEnabled(true);
    }

    std::string KeyViewer::KeyToJSON(const std::string& source, const std::string& destination, const remote::SharedKey& key)
    {
        auto keyData = QByteArray(key.keyvalue().data(), static_cast<int>(key.keyvalue().size()));

        std::string jsonMessage = "{ \"keyid\": " + std::to_string(key.keyid()) + ", "
                                  "\"source\": \"" + source + "\", "
                                  "\"dest\": \"" + destination + "\", "
                                  "\"url\": \"" + key.url() + "\", "
                                  "\"keyvalue\": \"" + keyData.toBase64().toStdString() + "\"}";
        return  jsonMessage;
    }

    QPixmap KeyViewer::GenerateQrCode(const std::string& jsonMessage)
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        const auto purple = QColor(0x44, 0x00, 0x64);
        const auto red = QColor(0xB0, 0x1C, 0x2E);

        LOGDEBUG(jsonMessage);
        // to get the colour gradient, fill an image with the gradient then mast it out with the qr code

        auto qrCode = QRcode_encodeData(static_cast<int>(jsonMessage.size()),
                                        reinterpret_cast<const unsigned char*>(jsonMessage.data()), 0, QRecLevel::QR_ECLEVEL_H);
        // the qr image is stored a one byte per pixel but only the LSB bit represents the pixel, the rest is usless info about the creation of the pixel
        // to be usful we need to convert it into pixels
        const uint numPixels = static_cast<uint>(qrCode->width) * static_cast<uint>(qrCode->width);
        for(uint index = 0; index < numPixels; index++)
        {
            if(qrCode->data[index] & 0x01)
            {
                // black
                qrCode->data[index] = 0x00;
            }
            else
            {
                // White
                qrCode->data[index] = 0xFF;
            }
        }

        QBitmap qrImage = QBitmap::fromImage(QImage(qrCode->data, qrCode->width, qrCode->width, qrCode->width, QImage::Format::Format_Grayscale8));
        qrImage = qrImage.scaledToWidth(qrCode->width*4);

        // create an image with the same size as the qr code
        QPixmap result(qrImage.size());
        result.fill(Qt::white);

        QLinearGradient grad(0.0, 0.0, 0.0, qrImage.height());

        grad.setColorAt(0.0, purple);
        grad.setColorAt(1.0, red);
        // fill the image with the gradient
        QBrush fillBrush(grad);
        // create a painter to fill in the pixmap
        QPainter painter(&result);

        // mask out the gradient with the qr code
        painter.setClipRegion(QRegion(qrImage));
        // fill the image
        painter.fillRect(0, 0, qrImage.width(), qrImage.height(), fillBrush);

        QRcode_free(qrCode);
        QApplication::restoreOverrideCursor();
        return result;
    }


    void KeyViewer::on_toSite_itemSelectionChanged()
    {
        const auto itemSelected = !ui->toSite->selectedItems().empty();
        ui->newKey->setEnabled(itemSelected);
        ui->existingKey->setEnabled(itemSelected);
    }

    void KeyViewer::on_refreshToList_clicked()
    {
        SetSourceSite(ui->fromSite->text().toStdString());
    }

    void KeyViewer::GetNewKey()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        auto stub = remote::IKey::NewStub(siteFromChannel);
        grpc::ClientContext ctx;
        remote::KeyRequest request;

        keyData = std::make_unique<remote::SharedKey>();
        request.set_siteto(ui->toSite->currentItem()->text().toStdString());

        // request a key
        auto getResult = stub->GetSharedKey(&ctx, request, keyData.get());

        if(LogStatus(getResult).ok())
        {
            DisplayKey(*keyData);
        }
        else
        {
            rekeyTimer.stop();
            QMessageBox::critical(this, QString::fromStdString("Failed to get key"),
                                  QString::fromStdString(getResult.error_message()));
        }
        QApplication::restoreOverrideCursor();
    }

    void KeyViewer::on_newKey_clicked()
    {
        if(siteFromChannel && ui->toSite->currentItem())
        {
            if(!ui->newKey->isCheckable() || ui->newKey->isChecked())
            {
                GetNewKey();
                if(ui->newKey->isCheckable())
                {
                    rekeyTimer.start();
                }
            }
            else
            {
                rekeyTimer.stop();
            }
        }
    }

    void KeyViewer::on_existingKey_clicked()
    {
        if(siteFromChannel)
        {
            auto existingKeyId = QInputDialog::getInt(this, "Key ID to get", "ID");
            if(existingKeyId > 0)
            {
                auto stub = remote::IKey::NewStub(siteFromChannel);
                if(stub)
                {
                    grpc::ClientContext ctx;
                    remote::KeyRequest request;
                    remote::SharedKey keyResponse;

                    request.set_siteto(ui->toSite->currentItem()->text().toStdString());
                    request.set_keyid(static_cast<uint64_t>(existingKeyId));

                    auto getResult = stub->GetSharedKey(&ctx, request, &keyResponse);

                    if(LogStatus(getResult).ok())
                    {
                        DisplayKey(keyResponse);
                    }
                    else
                    {
                        QMessageBox::critical(this, QString::fromStdString("Failed to get key"),
                                              QString::fromStdString(getResult.error_message()));
                    }
                }
            }
        }
    }

    void KeyViewer::on_formatBase64_clicked()
    {
        if(keyData)
        {
            // for converting the key data for display
            auto rawKey = QByteArray(keyData->keyvalue().data(), static_cast<int>(keyData->keyvalue().size()));
            ui->keyHexView->setPlainText(rawKey.toBase64());
        }
    }

    void cqp::KeyViewer::on_formatHex_clicked()
    {
        if(keyData)
        {
            auto rawKey = QByteArray(keyData->keyvalue().data(), static_cast<int>(keyData->keyvalue().size()));
            ui->keyHexView->setPlainText(rawKey.toHex());
        }
    }

    void KeyViewer::on_saveQr_clicked()
    {
        auto image = ui->qrCodeView->pixmap();
        if(image)
        {
            bool result = false;
            const auto pngFilter = tr("PNG Image (*.png)");
            const auto jsonFilter = tr("JSON Text (*.json)");
            const auto filter = pngFilter + tr(";;") + jsonFilter;

            QFileDialog saveDialog(this, "Save As...", tr(""), filter);
            saveDialog.setAcceptMode(QFileDialog::AcceptSave);
            if(saveDialog.exec() == QDialog::Accepted)
            {
                QApplication::setOverrideCursor(Qt::WaitCursor);

                auto filename = saveDialog.selectedFiles().first();
                if(saveDialog.selectedNameFilter() == pngFilter)
                {
                    if(!filename.endsWith(".png"))
                    {
                        filename += ".png";
                    }

                    result = image->save(filename, "PNG");
                }
                else
                {
                    if(!filename.endsWith(".json"))
                    {
                        filename += ".json";
                    }

                    if(keyData)
                    {
                        result = fs::WriteEntireFile(filename.toStdString(),
                                                     KeyToJSON(ui->fromSite->text().toStdString(),
                                                               ui->toSite->currentItem()->text().toStdString(), *keyData));
                    }
                }
                QApplication::restoreOverrideCursor();

                if(!result)
                {
                    QMessageBox::critical(this, "Failed to save", "Failed to save the key to\n" + filename);
                }
            }
            QApplication::restoreOverrideCursor();

        }
    }

    void KeyViewer::on_testQr_clicked()
    {
        keyData = std::make_unique<remote::SharedKey>();
        keyData->set_url("Siteb:8000");
        keyData->set_keyid(1234);
        keyData->set_keyvalue(QByteArray::fromBase64("9CqndMa11eureWq9n/LljgUwhpiV0ckhzX0fhzlDCjc=").toStdString());
        DisplayKey(*keyData);
    }

    void KeyViewer::on_refreshEnable_stateChanged(int state)
    {
        ui->newKey->setCheckable(state == Qt::Checked);
    }

    void KeyViewer::on_refreshTime_valueChanged(int newValue)
    {
        using std::chrono::seconds;

        rekeyTimer.setInterval(seconds(newValue));
    }

    void KeyViewer::OnKeyRefresh()
    {
        GetNewKey();
    }

} // namespace cqp
