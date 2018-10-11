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
#include "HSMPinDialog.h"

namespace keyviewer
{

    HSMPinDialog::HSMPinDialog(QWidget* parent, QString tokenName) :
        QDialog(parent)
    {
        setupUi(this);
        tokenLabel->setText(tokenName);
    }

    QString HSMPinDialog::GetPassword() const
    {
        return password->text();
    }

    cqp::keygen::UserType HSMPinDialog::GetUserType() const
    {
        using cqp::keygen::UserType;
        UserType result = UserType::User;
        if(userTypeUser->isChecked())
        {
            result = UserType::User;
        }
        else if(userTypeSO->isChecked())
        {
            result = UserType::SecurityOfficer;
        }
        else if(userTypeCS->isChecked())
        {
            result = UserType::ContextSpecific;
        }
        return result;
    }

} // namespace qkdtunnel
