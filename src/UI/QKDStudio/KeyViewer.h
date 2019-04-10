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
#ifndef CQP_KEYVIEWER_H
#define CQP_KEYVIEWER_H

#include <QDialog>
#include <memory>

namespace grpc
{
    class Channel;
    class ChannelCredentials;
}
namespace cqp
{

    namespace remote
    {
        class SharedKey;
    }
    namespace Ui
    {
        class KeyViewer;
    }

    class KeyViewer : public QDialog
    {
        Q_OBJECT

    public:
        explicit KeyViewer(QWidget *parent = nullptr, std::shared_ptr<grpc::ChannelCredentials> credentials = nullptr);
        ~KeyViewer();

        void SetSourceSite(const std::string& siteFrom);

        QStringList GetDestinations();

        void DisplayKey(const remote::SharedKey& key);

        static QPixmap GenerateQrCode(const std::string& source, const std::string& dest, uint64_t id, const QByteArray& value);

    private slots:
        void on_toSite_itemSelectionChanged();

        void on_refreshToList_clicked();

        void on_newKey_clicked();


        void on_formatBase64_clicked();

        void on_formatHex_clicked();

        void on_existingKey_clicked();

        void on_saveQr_clicked();

        void on_testQr_clicked();

    private:
        Ui::KeyViewer *ui;
        std::shared_ptr<grpc::Channel> siteFromChannel;
        std::shared_ptr<grpc::ChannelCredentials> creds;
        QPixmap qrCodeImage;

        enum FormatGroupIds
        {
            hex = 1,
            base64 = 2
        };

        QByteArray keyData;
    };


} // namespace cqp
#endif // CQP_KEYVIEWER_H
