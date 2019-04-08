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
#ifndef SITEEDITOR_H
#define SITEEDITOR_H

#include <QDialog>
#include "QKDInterfaces/Site.pb.h"

namespace cqp
{

    namespace Ui
    {
        class SiteEditor;
    }

    /**
     * @brief The SiteEditor class edits the site details
     */
    class SiteEditor : public QDialog
    {
        Q_OBJECT

    public:
        explicit SiteEditor(QWidget *parent = nullptr);
        ~SiteEditor();

        void SetConfig(const remote::SiteAgentConfig& config);
        void UpdateSite(remote::SiteAgentConfig& details);

        void ResetGui();
    private slots:
        void on_siteName_editingFinished();

        void on_id_editingFinished();

        void on_manager_editingFinished();

        void on_backingStore_editingFinished();

        void on_autoDiscovery_stateChanged(int arg1);

        void on_fallbackKey_editingFinished();

        void on_exportConfig_clicked();

    private:
        Ui::SiteEditor *ui;
        remote::SiteAgentConfig editing;
    };

}
#endif // SITEEDITOR_H
