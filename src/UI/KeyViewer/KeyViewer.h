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
#pragma once

#include <QMainWindow>
#include "CQPToolkit/Interfaces/IService.h"
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "CQPToolkit/KeyGen/HSMStore.h"
#include <QMenu>
#include "CQPUI/HSMPinDialog.h"

namespace Ui
{
    class KeyViewer;
}

/**
 * @brief The MainWindow class
 */
class KeyViewer : public QMainWindow, public virtual cqp::IServiceCallback
{
    Q_OBJECT

public:
    /// constructor
    /// @param parent Parent widget
    explicit KeyViewer(QWidget *parent = nullptr);
    /// Distructor
    ~KeyViewer() override;

private:
    /// control access to the local site agents list
    std::mutex localSiteAgentsMutex;
    /// the window
    Ui::KeyViewer *ui;
    /// detect services
    cqp::net::ServiceDiscovery sd;
    /// channels for connections
    std::shared_ptr<grpc::Channel> channel;
    QByteArray keyData;
    uint64_t keyId = 0;

    QMenu hsmMenu;
    cqp::ui::HSMPinDialog pinDialog;

    std::vector<std::string> knownModules = {"libsofthsm2.so", "yubihsm_pkcs11.so"};
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

    void on_localAgentGo_clicked();
    void on_addModule_clicked();
    void on_clearHSM_clicked();
    void on_openHsm_clicked();
    void on_sendToHsm_clicked();
    void on_openDestHsm_clicked();
};
