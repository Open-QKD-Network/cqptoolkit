/*!
* @file
* @brief DeviceDialog
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "ui_DeviceDialog.h"
#include <QDialog>
#include "CQPToolkit/Util/URI.h"

namespace qkdtunnel
{

    /**
     * @brief The DeviceDialog class
     */
    class DeviceDialog : public QDialog, protected Ui::DeviceDialog
    {
    public:

        Q_OBJECT

    public:
        /**
         * @brief DeviceDialog
         * @param parent
         */
        DeviceDialog(QWidget * parent);
        /**
         * @brief SetData
         * @param uri
         */
        void SetData(const QString& uri);
        /**
         * @brief GetUri
         * @return The uri
         */
        QString GetUri();

    protected:

        /// The uri of the device
        cqp::URI uri;

    private slots:
        /// Handle user change
        void on_deviceNetmask_editingFinished();
        /// Handle user change
        void on_deviceName_editingFinished();
        /// Handle user change
        void on_dataPortProm_stateChanged(int);
        /// Handle user change
        void on_dataPortCapture_currentTextChanged(const QString &);
        /// Handle user change
        /// @param arg1
        void on_dataPortType_currentTextChanged(const QString &arg1);
        /// Handle user change
        void on_dataPortPort_valueChanged(int);
        /// Handle user change
        void on_dataPortAddress_editingFinished();
        /// Handle user change
        void on_deviceUri_editingFinished();
    }; // class DeviceDialog

} // namespace qkdtunnel


