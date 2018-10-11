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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CQPToolkit/Interfaces/IService.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "CQPToolkit/KeyGen/HSMStore.h"
#include <QMenu>

namespace Ui
{
    class MainWindow;
}

namespace keyviewer
{

    /**
     * @brief The MainWindow class
     */
    class MainWindow : public QMainWindow, public virtual cqp::IServiceCallback, public virtual cqp::keygen::IPinCallback
    {
        Q_OBJECT

    public:
        /// constructor
        /// @param parent Parent widget
        explicit MainWindow(QWidget *parent = nullptr);
        /// Distructor
        ~MainWindow() override;

    private:
        /// control access to the local site agents list
        std::mutex localSiteAgentsMutex;
        /// the window
        Ui::MainWindow *ui;
        /// detect services
        cqp::net::ServiceDiscovery sd;
        /// channels for connections
        std::shared_ptr<grpc::Channel> channel;
        QByteArray keyData;
        uint64_t keyId = 0;

        QMenu hsmMenu;

        // IServiceCallback interface
    public:
        void OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices) override;

        /**
         * @brief ClearKey
         * Delete the key material from the gui
         */
        void ClearKey();

    private slots:
        void on_clearKey_clicked();

    private slots:
        /// handle user interaction
        void on_getExistingKey_clicked();
        /// handle user interaction
        void on_getNewKey_clicked();
        /// handle user interaction
        void on_knownSites_currentRowChanged(int currentRow);
        void on_revealKey_clicked();
        void OnSendToHSMShow();
        void OnSendToHSMHide();
        void HSMPicked(QAction* action);

        // IPinCallback interface
    public:
        bool GetHSMPin(const std::string& tokenSerial, const std::string& tokenLabel, cqp::keygen::UserType& login, std::string& pin) override;
    private slots:
        void on_localAgentGo_clicked();
    };

} // namespace keyviewer
#endif // MAINWINDOW_H
