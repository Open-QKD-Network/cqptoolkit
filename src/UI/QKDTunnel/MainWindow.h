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
#pragma once

#include <QMainWindow>
#include <QStringListModel>
#include "CQPToolkit/Net/ServiceDiscovery.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Tunnels/Controller.h"
#include "ControllerModel.h"
#include <QDataWidgetMapper>
#include <QFileDialog>
#include <QAbstractButton>
#include "KeyStoreModel.h"

namespace Ui
{
    class MainWindow;
}

namespace qkdtunnel
{
    class DeviceDialog;

    /**
     * @brief The MainWindow class
     * Main window for the QKD Tunnel program
     */
    class MainWindow : public QMainWindow, public virtual cqp::IServiceCallback
    {
        Q_OBJECT

    public:
        /**
         * @brief MainWindow
         * Constructor
         * @param parent
         */
        explicit MainWindow(QWidget *parent = nullptr);

        /**
         * @brief ~MainWindow
         * Destructor
         */
        virtual ~MainWindow() override;
    protected:
        int uniqueCounter = 1;

    private slots:
        /// Handle user change
        void on_certUseTls_stateChanged(int arg1);
        /// Handle user change
        void on_caFile_editingFinished();
        /// Handle user change
        void on_keyFile_editingFinished();
        /// Handle user change
        void on_certFile_editingFinished();
        /// Handle user change
        void on_tunStartDevice_editingFinished();
        /// Handle user change
        void on_tunEndDevice_editingFinished();
        /// Handle user change
        void on_keyStoreFactory_editTextChanged(const QString &arg1);
        /// Handle user change
        void on_tunOtherController_editTextChanged(const QString &arg1);
        /// Handle user change
        void on_keyStoreFactory_currentIndexChanged(int index);
        /// Handle user change
        void on_tunOtherController_currentIndexChanged(int index);
        /// Handle user change
        void on_tunEndDeviceEdit_clicked();
        /// Handle user change
        void on_tunStartDeviceEdit_clicked();
        /// Handle user change
        void on_tunKeyMaxTime_valueChanged(int arg1);
        /// Handle user change
        void on_tunKeyMaxTimeUnits_currentIndexChanged(int index);
        /// Handle user change
        void on_tunKeyMaxBytesScale_currentIndexChanged(int index);
        /// Handle user change
        void on_tunCryptoKeySize_currentTextChanged(const QString &arg1);
        /// Handle user change
        void on_tunCryptoBlockCypher_currentTextChanged(const QString &arg1);
        /// Handle user change
        void on_tunCryptoSubMode_currentTextChanged(const QString &arg1);
        /// Handle user change
        void on_tunCryptoMode_currentTextChanged(const QString &arg1);
        /// Handle user change
        void on_tunKeyMaxBytes_valueChanged(int arg1);
        /// Handle user change
        void on_listenPort_editingFinished();
        /// Handle user change
        void on_tunActivate_clicked(bool active);
        /// Handle user change
        void on_clearModels_clicked();
        /// Handle user change
        void on_tunName_editingFinished();
        /// Handle user change
        void on_exportSettings_clicked();
        /// Handle user change
        void on_importSettings_clicked();
        /// Handle user change
        void on_controllerHost_editingFinished();
        /// Handle user change
        void on_controllerName_editingFinished();
        /// Handle user change
        void on_controllerDelete_clicked();
        /// Handle user change
        void on_createTunnel_clicked();
        /// Handle user change
        void on_controllerAdd_clicked();
        /// Handle user change
        void on_manualConnect_clicked();

    private:
        /**
         * @brief BuildDeviceURI
         * @param item
         * @param sourceString
         * @return
         */
        QUrl BuildDeviceURI(DeviceItem* item, const QString& sourceString);

        /**
         * @brief DecodeURI
         * @param uri
         */
        void DecodeURI(const QUrl& uri);

        /**
         * @brief ControllerSelectionChanged
         * @param current
         * @param previous
         */
        void ControllerSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

        /**
         * @brief OnServiceDetected
         * @param newServices
         * @param deletedServices
         */
        void OnServiceDetected(const cqp::RemoteHosts &newServices, const cqp::RemoteHosts &deletedServices) override;

        /**
         * @brief DeviceUrlEditingFinished
         */
        void DeviceUrlEditingFinished();

        /**
         * @brief KeyExchangeTypeEditingFinished
         */
        void KeyExchangeTypeEditingFinished();

        /**
         * @brief KeyStoreByIdClicked
         * @param btn
         */
        void KeyStoreByIdClicked(QAbstractButton* btn);
        /**
         * @brief TunOtherControllerByIdClicked
         * @param btn
         */
        void TunOtherControllerByIdClicked(QAbstractButton* btn);
        /**
         * @brief ui
         */
        Ui::MainWindow *ui;

        /**
         * @brief servDiscovery
         */
        cqp::net::ServiceDiscovery servDiscovery;
        /**
         * @brief controllermodel
         */
        ControllerModel controllermodel;

        KeyStoreModel keyStoresModel;
        QStringListModel tunnelServersModel;
        /**
         * @brief settingsSaveDialog
         */
        QFileDialog settingsSaveDialog;

        DeviceDialog* deviceDialog = nullptr;
    }; // class MainWindow
} // namespace qkdtunnel
