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
#include <QDialog>
#include "CQPToolkit/KeyGen/HSMStore.h"
#include "CQPUI/cqpui_export.h"

namespace Ui
{
    class HSMPinDialog;
}

namespace cqp
{
namespace ui
{
    /**
     * @brief The DeviceDialog class
     */
    class CQPUI_EXPORT HSMPinDialog : public QDialog, public virtual cqp::keygen::IPinCallback
    {
    public:

        Q_OBJECT

    public:
        /**
         * @brief DeviceDialog
         * @param parent
         */
        explicit HSMPinDialog(QWidget * parent);

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

        /// @copydoc cqp::keygen::IPinCallback::GetHSMPin
        bool GetHSMPin(const std::string& tokenSerial,
                       const std::string& tokenName,
                       cqp::keygen::UserType& login,
                       std::string& pin);
    protected:
        ::Ui::HSMPinDialog* ui = nullptr;
    }; // class DeviceDialog
} // namespace ui
} // namespace cqp


