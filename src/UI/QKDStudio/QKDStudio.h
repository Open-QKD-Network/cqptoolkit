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
#pragma once
#include <QMainWindow>
#include <memory>
#include <grpcpp/security/credentials.h>

class QGraphicsItem;

namespace QtNodes
{
    class Connection;
}

namespace cqp
{
    class ConnectDialog;

    namespace Ui
    {
        class QKDStudio;
    }

    namespace ui
    {
        class QKDNodeEditor;

        /**
         * @brief The QKDStudio class
         * GUI for controlling and designing QKD systems
         */
        class QKDStudio : public QMainWindow
        {
        public:
            /**
             * @brief QKDStudio
             * @param parent
             * @param flags
             */
            QKDStudio(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

            /// Destructor
            ~QKDStudio() override;
        protected slots:

            void OnSelectionChanged();
            void OnConnectTo();
            void GrpcConnectionFinished(int result);
            void ConnectionCreated(QtNodes::Connection const & conn);
            void ShowKeyViewer();
        protected:
            bool AddLiveSiteAgent(const std::string& address);
            bool AddLiveDevice(const std::string& address);
            bool AddLiveManager(const std::string& address);

        private:
            std::unique_ptr<Ui::QKDStudio> ui;
            std::unique_ptr<ConnectDialog> connectDialog;
            std::unique_ptr<QKDNodeEditor> nodeData;
            std::shared_ptr<grpc::ChannelCredentials> creds;
        };

    }
}

