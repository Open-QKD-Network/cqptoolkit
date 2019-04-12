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
#include <QTimer>
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

    /**
     * @brief The KeyViewer class provides a dialog for getting keys from the IKey interface
     */
    class KeyViewer : public QDialog
    {
        Q_OBJECT

    public:
        /**
         * @brief KeyViewer Constructor
         * @param parent
         * @param credentials
         */
        KeyViewer(QWidget *parent = nullptr, std::shared_ptr<grpc::ChannelCredentials> credentials = nullptr);

        /// Destructor
        ~KeyViewer();

        /**
         * @brief SetSourceSite
         * Set the address to connect to
         * @param siteFrom connection address
         */
        void SetSourceSite(const std::string& siteFrom);

        /**
         * @brief GetDestinations
         * @return A list of sites which the source has key stores for
         */
        QStringList GetDestinations();

        /**
         * @brief DisplayKey
         * Render a key on the dialog
         * @param key Key data to display
         */
        void DisplayKey(const remote::SharedKey& key);

        /**
         * @brief GenerateQrCode
         * Display the key data as a QR code
         * @details
         * The values are encoded as a json string:
         * @code
         * {
         *   "keyid": 1234,
         *   "source": "source",
         *   "dest": "dest",
         *   "keyvalue": "<base64 string>"
         * }
         * @endcode
         * @param source Site address for the source
         * @param dest Site address for the destination field
         * @param id key id
         * @param value key data
         * @return the QWR code as an image
         */
        static QPixmap GenerateQrCode(const std::string& jsonMessage);

        /**
         * @brief GetNewKey
         * Request a new key from the source and display it
         */
        void GetNewKey();

        /**
         * @brief KeyToJSON Turn the key data into json
         * @details
         * The values are encoded as a json string:
         * @code
         * {
         *   "keyid": 1234,
         *   "source": "source",
         *   "dest": "dest",
         *   "keyvalue": "<base64 string>"
         * }
         * @endcode
         * @param source the source of the key
         * @param key the key data
         * @return a json string
         */
        static std::string KeyToJSON(const std::string& source, const remote::SharedKey& key);

    private slots:
        void on_toSite_itemSelectionChanged();

        void on_refreshToList_clicked();

        void on_newKey_clicked();


        void on_formatBase64_clicked();

        void on_formatHex_clicked();

        void on_existingKey_clicked();

        void on_saveQr_clicked();

        void on_testQr_clicked();

        void on_refreshEnable_stateChanged(int state);

        void on_refreshTime_valueChanged(int newValue);

        void OnKeyRefresh();
    private:
        std::unique_ptr<Ui::KeyViewer> ui;
        std::shared_ptr<grpc::Channel> siteFromChannel;
        std::shared_ptr<grpc::ChannelCredentials> creds;
        QPixmap qrCodeImage;

        enum FormatGroupIds
        {
            hex = 1,
            base64 = 2
        };

        std::unique_ptr<remote::SharedKey> keyData;
        QTimer rekeyTimer;
    };


} // namespace cqp
#endif // CQP_KEYVIEWER_H
