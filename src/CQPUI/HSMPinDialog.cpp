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
#include "CQPUI/CQPUI_autogen/include/ui_HSMPinDialog.h"
#include "HSMPinDialog.h"

namespace cqp
{
namespace ui
{

    HSMPinDialog::HSMPinDialog(QWidget* parent) :
        QDialog(parent),
        ui(new Ui::HSMPinDialog)
    {
        ui->setupUi(this);
    }

    QString HSMPinDialog::GetPassword() const
    {
        return ui->password->text();
    }

    cqp::keygen::UserType HSMPinDialog::GetUserType() const
    {
        using cqp::keygen::UserType;
        UserType result = UserType::User;
        if(ui->userTypeUser->isChecked())
        {
            result = UserType::User;
        }
        else if(ui->userTypeSO->isChecked())
        {
            result = UserType::SecurityOfficer;
        }
        else if(ui->userTypeCS->isChecked())
        {
            result = UserType::ContextSpecific;
        }
        return result;
    }

    bool HSMPinDialog::GetHSMPin(const std::string& tokenSerial, const std::string& tokenName, cqp::keygen::UserType& login, std::string& pin)
    {
        bool result = false;
        ui->tokenLabel->setText(QString::fromStdString(tokenName + "(" + tokenSerial + ")"));
        if(exec() == QDialog::Accepted)
        {
            login = GetUserType();
            pin = GetPassword().toStdString();
            result = true;
        }

        return result;
    }

} // namespace ui
} // namespace cqp
