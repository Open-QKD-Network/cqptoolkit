/*!
* @file
* @brief DeviceDialog
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 17/08/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "ui_HSMPinDialog.h"
#include <QDialog>
#include "CQPToolkit/KeyGen/HSMStore.h"

namespace keyviewer
{

    /**
     * @brief The DeviceDialog class
     */
    class HSMPinDialog : public QDialog, protected Ui::HSMPinDialog
    {
    public:

        Q_OBJECT

    public:
        /**
         * @brief DeviceDialog
         * @param parent
         */
        HSMPinDialog(QWidget * parent, QString tokenName);

        /**
         * @brief GetPassword
         * @return The password provided by the user
         */
        QString GetPassword() const;

        /**
         * @brief GetUserType
         * @return the user type
         */
        cqp::keygen::UserType GetUserType() const;

    }; // class DeviceDialog

} // namespace keyviewer


