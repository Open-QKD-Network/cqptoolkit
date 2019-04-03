/*!
* @file
* @brief cqp::ConnectDialog
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ConnectDialog.h"
#include "ui_ConnectDialog.h"

namespace cqp
{

    ConnectDialog::ConnectDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::ConnectDialog)
    {
        ui->setupUi(this);
        ui->serviceGroup->setId(ui->typeSite, 1);
        ui->serviceGroup->setId(ui->typeDevice, 2);
        ui->serviceGroup->setId(ui->typeManager, 3);
    }

    ConnectDialog::~ConnectDialog()
    {
        delete ui;
    }

    ConnectDialog::ConnectionType ConnectDialog::GetType()
    {
        ConnectDialog::ConnectionType result = ConnectionType::Site;
        switch (ui->serviceGroup->checkedId())
        {
        case 1:
            result = ConnectionType::Site;
            break;
        case 2:
            result = ConnectionType::Device;
            break;
        case 3:
            result =ConnectionType::Manager;
            break;
        }
        return result;
    }

    std::string ConnectDialog::GetAddress()
    {
        return ui->address->text().toStdString();
    }

} // namespace cqp
