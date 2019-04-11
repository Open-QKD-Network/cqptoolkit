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
// qr stuff
#include <qrencode.h>

using google::protobuf::Empty;
using grpc::Status;
using grpc::StatusCode;

namespace cqp
{

    KeyViewer::KeyViewer(QWidget *parent, std::shared_ptr<grpc::ChannelCredentials> credentials) :
        QDialog(parent),
        ui(new Ui::KeyViewer)
    {
        ui->setupUi(this);

        ui->formatGroup->setId(ui->formatHex, FormatGroupIds::hex);
        ui->formatGroup->setId(ui->formatBase64, FormatGroupIds::base64);

        creds = credentials;
        if(!creds)
        {
            creds = grpc::InsecureChannelCredentials();
        }
    }

    KeyViewer::~KeyViewer()
    {
        delete ui;
    }

    void KeyViewer::SetSourceSite(const std::string& siteFrom)
    {
        ui->fromSite->setText(QString::fromStdString(siteFrom));
        siteFromChannel = grpc::CreateChannel(siteFrom, creds);
        ui->toSite->clear();
        ui->toSite->addItems(GetDestinations());
    }

    QStringList KeyViewer::GetDestinations()
    {
        QStringList result;
        auto stub = remote::IKey::NewStub(siteFromChannel);
        if(stub)
        {
            grpc::ClientContext ctx;
            remote::SiteList sites;
            auto getResult = stub->GetKeyStores(&ctx, Empty(), &sites);
            if(LogStatus(getResult).ok())
            {
                for(const auto& remoteSite : sites.urls())
                {
                    result.push_back(QString::fromStdString(remoteSite));
                }
            }
            else
            {
                QMessageBox::critical(this, QString::fromStdString("Failed to connect"),
                                      QString::fromStdString(getResult.error_message()));
            }
        }

        return result;
    }

    void KeyViewer::DisplayKey(const remote::SharedKey& key)
    {
        ui->id->setText(QString::fromStdString(std::to_string(key.keyid())));
        keyData = QByteArray(key.keyvalue().data(), key.keyvalue().size());

        switch (ui->formatGroup->checkedId())
        {
        case FormatGroupIds::hex:
            ui->keyHexView->setPlainText(keyData.toHex());
            break;
        case FormatGroupIds::base64:
            ui->keyHexView->setPlainText(keyData.toBase64());
            break;
        }

        qrCodeImage = GenerateQrCode(ui->fromSite->text().toStdString(), key.url(),
                                     key.keyid(), keyData);
        ui->qrCodeView->setPixmap(qrCodeImage.scaled(ui->qrCodeView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->saveQr->setEnabled(true);
    }

    QPixmap KeyViewer::GenerateQrCode(const std::string& source, const std::string& dest,
                                      uint64_t id, const QByteArray& value)
    {
        const auto purple = QColor(0x44, 0x00, 0x64);
        const auto red = QColor(0xB0, 0x1C, 0x2E);

        std::string jsonMessage = "{ \"keyid\": " + std::to_string(id) + ", "
                                  "\"source\": \"" + source + "\", "
                                  "\"dest\": \"" + dest + "\", "
                                  "\"keyvalue\": \"" + value.toBase64().toStdString() + "\"}";

        LOGDEBUG(jsonMessage);
        // to get the colour gradient, fill an image with the gradient then mast it out with the qr code

        auto qrCode = QRcode_encodeData(jsonMessage.size(), reinterpret_cast<const unsigned char*>(jsonMessage.data()), 0, QRecLevel::QR_ECLEVEL_H);
        // the qr image is stored a one byte per pixel but only the LSB bit represents the pixel, the rest is usless info about the creation of the pixel
        // to be usful we need to convert it into pixels
        const uint numPixels = qrCode->width * qrCode->width;
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

    void KeyViewer::on_newKey_clicked()
    {
        auto stub = remote::IKey::NewStub(siteFromChannel);
        if(stub)
        {
            grpc::ClientContext ctx;
            remote::KeyRequest request;
            remote::SharedKey keyResponse;

            request.set_siteto(ui->toSite->currentItem()->text().toStdString());

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

    void KeyViewer::on_existingKey_clicked()
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
                request.set_keyid(existingKeyId);

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

    void KeyViewer::on_formatBase64_clicked()
    {
        ui->keyHexView->setPlainText(keyData.toBase64());
    }

    void cqp::KeyViewer::on_formatHex_clicked()
    {
        ui->keyHexView->setPlainText(keyData.toHex());
    }

    void KeyViewer::on_saveQr_clicked()
    {
        auto image = ui->qrCodeView->pixmap();
        if(image)
        {
            auto saveFile = QFileDialog::getSaveFileName(this, "Save As...");
            image->save(saveFile, "PNG");
        }
    }

    void KeyViewer::on_testQr_clicked()
    {
        remote::SharedKey key;
        key.set_url("Siteb:8000");
        key.set_keyid(1234);
        key.set_keyvalue(QByteArray::fromBase64("9CqndMa11eureWq9n/LljgUwhpiV0ckhzX0fhzlDCjc=").toStdString());
        DisplayKey(key);
    }

} // namespace cqp
