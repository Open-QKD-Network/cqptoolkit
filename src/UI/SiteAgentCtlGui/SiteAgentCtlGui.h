/*!
* @file
* @brief SiteAgentCtlGui
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/10/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <QMainWindow>
#include <QTreeWidgetItem>
#include "CQPToolkit/Net/ServiceDiscovery.h"

namespace Ui {
    class SiteAgentCtlGui;
}

namespace cqp {
    namespace remote
    {
        class PhysicalPath;
    }
}
class SiteAgentCtlGui : public QMainWindow, public virtual cqp::IServiceCallback
{
    Q_OBJECT

public:
    explicit SiteAgentCtlGui(QWidget *parent = nullptr);
    virtual ~SiteAgentCtlGui() override;

    cqp::remote::PhysicalPath GetHops();

    // IServiceCallback interface
public:
    void OnServiceDetected(const cqp::RemoteHosts& newServices, const cqp::RemoteHosts& deletedServices) override;

    void AddSite(const std::string& address);

private slots:
    void on_addSite_clicked();

    void on_removeSite_clicked();

    void on_addDeviceFrom_clicked();

    void on_siteTree_itemClicked(QTreeWidgetItem *item, int column);
    void on_removeDevice_clicked();

    void on_getJson_clicked();

    void on_devicesInHop_itemClicked(QTreeWidgetItem *item, int column);

    void on_addDeviceTo_clicked();

    void on_createLink_clicked();

    void on_clearHops_clicked();

    void on_clearSites_clicked();

    void on_stopLink_clicked();

private:
    Ui::SiteAgentCtlGui *ui;
    /// control access to the local site agents list
    std::mutex localSiteAgentsMutex;
};
