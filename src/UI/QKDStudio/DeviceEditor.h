/*!
* @file
* @brief cqp::DeviceEditor
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#ifndef CQP_DEVICEEDITOR_H
#define CQP_DEVICEEDITOR_H

#include <QDialog>
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{

    namespace Ui
    {
        class DeviceEditor;
    }

    class DeviceEditor : public QDialog
    {
        Q_OBJECT

    public:
        explicit DeviceEditor(QWidget *parent = nullptr);
        ~DeviceEditor();

        void SetDetails(const remote::ControlDetails& details);
        void UpdateDetails(remote::ControlDetails& details);
        void ResetGui();
    private slots:
        void on_controlAddress_editingFinished();

        void on_siteAgent_editingFinished();

        void on_Id_editingFinished();

        void on_side_currentIndexChanged(int index);

        void on_switchName_editingFinished();

        void on_switchPort_editingFinished();

        void on_kind_editingFinished();

        void on_bytesPerKey_currentIndexChanged(int index);

        void on_exportConfig_clicked();

    private:
        Ui::DeviceEditor *ui;
        remote::ControlDetails editing;
    };


} // namespace cqp
#endif // CQP_DEVICEEDITOR_H
