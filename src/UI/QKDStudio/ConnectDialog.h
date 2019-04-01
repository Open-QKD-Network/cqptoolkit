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
#ifndef CQP_CONNECTDIALOG_H
#define CQP_CONNECTDIALOG_H

#include <QDialog>

namespace cqp
{

    namespace Ui
    {
        class ConnectDialog;
    }

    class ConnectDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit ConnectDialog(QWidget *parent = nullptr);
        ~ConnectDialog();

        enum class ConnectionType { Site, Device, Manager };

        ConnectionType GetType();
        std::string GetAddress();

    private:
        Ui::ConnectDialog *ui;
    };


} // namespace cqp
#endif // CQP_CONNECTDIALOG_H
