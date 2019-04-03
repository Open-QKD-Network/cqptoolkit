/*!
* @file
* @brief SiteEditor
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteEditor.h"
#include "ui_SiteEditor.h"
#include "Algorithms/Util/Strings.h"

#define QS(x) QString::fromStdString(x)

namespace cqp
{

    SiteEditor::SiteEditor(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::SiteEditor)
    {
        ui->setupUi(this);
    }

    SiteEditor::~SiteEditor()
    {
        delete ui;
    }

    void SiteEditor::SetConfig(const remote::SiteAgentConfig& config)
    {
        editing = config;
        ResetGui();
    }

    void SiteEditor::ResetGui()
    {
        ui->siteName->setText(QS(editing.name()));
        ui->id->setText(QS(editing.id()));
        ui->manager->setText(QS(editing.netmanuri()));
        ui->listenPort->setText(QS(std::to_string(editing.listenport())));
        ui->bindAddress->setText(QS(editing.bindaddress()));
        ui->backingStore->setText(QS(editing.backingstoreurl()));
        ui->fallbackKey->setText(QS(ToHexString(editing.fallbackkey())));
    }

    void SiteEditor::UpdateSite(remote::SiteAgentConfig& details)
    {
        details = editing;
    }

    void SiteEditor::on_siteName_editingFinished()
    {
        editing.set_name(ui->siteName->text().toStdString());
    }

    void SiteEditor::on_id_editingFinished()
    {
        editing.set_id(ui->id->text().toStdString());
    }

    void SiteEditor::on_manager_editingFinished()
    {
        editing.set_netmanuri(ui->manager->text().toStdString());
    }

    void SiteEditor::on_backingStore_editingFinished()
    {
        editing.set_backingstoreurl(ui->backingStore->text().toStdString());
    }

    void SiteEditor::on_autoDiscovery_stateChanged(int arg1)
    {
        if(arg1 == 0)
        {
            editing.set_useautodiscover(false);
        }
        else
        {
            editing.set_useautodiscover(true);
        }
    }


    void SiteEditor::on_fallbackKey_editingFinished()
    {
        auto bytes = HexToBytes(ui->fallbackKey->text().toStdString());
        editing.mutable_fallbackkey()->resize(bytes.size());
        editing.mutable_fallbackkey()->assign(bytes.begin(), bytes.end());
    }


}
