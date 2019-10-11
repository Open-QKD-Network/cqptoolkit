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
#include "DeviceEditor.h"
#include "ui_DeviceEditor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <google/protobuf/util/json_util.h>
#include "Algorithms/Util/FileIO.h"

#define QS(x) QString::fromStdString(x)

namespace cqp
{

    DeviceEditor::DeviceEditor(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DeviceEditor)
    {
        ui->setupUi(this);
    }

    DeviceEditor::~DeviceEditor()
    {
        delete ui;
    }

    void DeviceEditor::SetDetails(const remote::ControlDetails& details)
    {
        editing = details;
        ResetGui();

    }

    void DeviceEditor::ResetGui()
    {
        ui->id->setText(QS(editing.config().id()));
        ui->kind->setText(QS(editing.config().kind()));
        if(editing.config().side() == remote::Side_Type::Side_Type_Alice)
        {
            ui->side->setCurrentIndex(0);
        }
        else
        {
            ui->side->setCurrentIndex(1);
        }
        ui->siteAgent->setText(QS(editing.siteagentaddress()));
        ui->switchName->setText(QS(editing.config().switchname()));
        ui->switchPort->setText(QS(Join(editing.config().switchport(), ",")));
        if(editing.config().bytesperkey() == 16)
        {
            ui->bytesPerKey->setCurrentIndex(0);
        }
        else
        {
            ui->bytesPerKey->setCurrentIndex(1);
        }
        ui->controlAddress->setText(QS(editing.controladdress()));

    }


    void DeviceEditor::UpdateDetails(remote::ControlDetails& details)
    {
        details = editing;
    }

    void DeviceEditor::on_controlAddress_editingFinished()
    {
        editing.set_controladdress(ui->controlAddress->text().toStdString());
    }

    void DeviceEditor::on_siteAgent_editingFinished()
    {
        editing.set_siteagentaddress(ui->siteAgent->text().toStdString());
    }

    void DeviceEditor::on_Id_editingFinished()
    {
        editing.mutable_config()->set_id(ui->id->text().toStdString());
    }

    void DeviceEditor::on_side_currentIndexChanged(int index)
    {
        if(index == 0)
        {
            editing.mutable_config()->set_side(remote::Side_Type::Side_Type_Alice);
        }
        else
        {
            editing.mutable_config()->set_side(remote::Side_Type::Side_Type_Bob);
        }
    }

    void DeviceEditor::on_switchName_editingFinished()
    {
        editing.mutable_config()->set_switchname(ui->switchName->text().toStdString());
    }

    void DeviceEditor::on_switchPort_editingFinished()
    {
        editing.mutable_config()->add_switchport(ui->switchPort->text().toStdString());
    }

    void DeviceEditor::on_kind_editingFinished()
    {
        editing.mutable_config()->set_kind(ui->kind->text().toStdString());
    }

    void DeviceEditor::on_bytesPerKey_currentIndexChanged(int index)
    {
        if(index == 0)
        {
            editing.mutable_config()->set_bytesperkey(16);
        }
        else
        {
            editing.mutable_config()->set_bytesperkey(32);
        }
    }

    void DeviceEditor::on_exportConfig_clicked()
    {

        QFileDialog dlg(this, "Save Device config");
        dlg.setDefaultSuffix(".json");
        if(dlg.exec() == QDialog::Accepted)
        {
            auto filenames = dlg.selectedFiles();
            using namespace google::protobuf::util;
            std::string jsonStr;
            JsonOptions options;
            options.add_whitespace = true;
            options.always_print_primitive_fields = true;
            auto result = MessageToJsonString(editing, &jsonStr);

            if(!result.ok())
            {
                QMessageBox::critical(this, "Failed to generate json",
                                      QString::fromStdString(result.error_message()));
            }
            else
            {
                if(!fs::WriteEntireFile(filenames[0].toStdString(), jsonStr))
                {
                    QMessageBox::critical(this, QString::fromStdString("Failed to write"),
                                          QString::fromStdString("Failed to export json to ") + filenames[0]);
                }
            }
        }
    }
} // namespace cqp

