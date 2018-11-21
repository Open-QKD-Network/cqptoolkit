/*!
* @file
* @brief ControllerModel
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ControllerModel.h"
#include <QStandardItem>
#include "CQPToolkit/Util/UUID.h"
#include "CQPToolkit/Util/Logger.h"
#include "QKDInterfaces/Tunnels.pb.h"
#include "QKDInterfaces/ITunnelServer.grpc.pb.h"

namespace qkdtunnel
{
    ControllerModel::ControllerModel(QObject *parent) : QStandardItemModel(parent)
    {
    }

    void ControllerModel::SetRemote(const cqp::RemoteHost& host)
    {
        using namespace std;
        using namespace cqp::remote::tunnels;

        if(host.interfaces.contains(ITunnelServer::service_full_name()))
        {
            // find any new services
            bool found = false;
            for(int row = 0; row < rowCount(); row++)
            {
                ControllerItem* controller = dynamic_cast<ControllerItem*>(item(row));
                if(controller && controller->GetId() == host.id)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                ControllerItem* newItem = new ControllerItem(cqp::UUID(host.id), QString::fromStdString(host.name));
                newItem->setData(host.port, ControllerItem::Index::listenPort);
                newItem->setData(QString::fromStdString(host.host), ControllerItem::Index::listenAddress);
                newItem->setData(QString::fromStdString(host.host + ":" + to_string(host.port)), ControllerItem::Index::connectionAddress);
                newItem->setData(true, ControllerItem::Index::running);
                appendRow(newItem);
            }
        }
    }

    void ControllerModel::SetRemote(const cqp::RemoteHosts& services)
    {
        using namespace std;
        using namespace cqp::remote::tunnels;

        for(const auto& host : services)
        {
            SetRemote(host.second);
        }
        // look for stale controllers
        for(int row = 0; row < rowCount(); row++)
        {
            ControllerItem* controller = dynamic_cast<ControllerItem*>(item(row));
            if(controller && services.find(controller->GetId()) == services.end())
            {
                //controller->SetIsRunning(false);
            }
        }
    }

    bool ControllerModel::RemoveController(const std::string& id)
    {
        bool result = false;
        for(int row = 0; row < rowCount(); row++)
        {
            ControllerItem* controller = dynamic_cast<ControllerItem*>(item(row));
            if(controller && controller->GetId() == id)
            {
                removeRow(row);
                result = true;
                break; // for
            }
        }
        return result;
    }

    ControllerItem* ControllerModel::GetController(const std::string& id) const
    {
        ControllerItem* result = nullptr;
        for(int row = 0; row < rowCount(); row++)
        {
            result = dynamic_cast<ControllerItem*>(item(row));
            if(result && result->GetId() == id)
            {
                break; // for
            }
            else
            {
                result = nullptr;
            }
        }
        return result;
    }

    bool ControllerModel::submit()
    {
        bool result = QAbstractItemModel::submit();
        if(result)
        {
            for(int row = 0; row < rowCount(); row++)
            {
                ControllerItem* controller = dynamic_cast<ControllerItem*>(item(row));
                if(!controller || !controller->Submit())
                {
                    LOGERROR("Failed to submit");
                }
                // come settings may have been reverted by the endpoint, refresh the display
                emit dataChanged(index(row, 0), index(row, 0));
            }
        }
        return result;
    }

    void ControllerModel::revert()
    {
        QAbstractItemModel::revert();
        for(int row = 0; row < rowCount(); row++)
        {
            ControllerItem* controller = dynamic_cast<ControllerItem*>(item(row));
            if(controller)
            {
                controller->Revert();
            }
        }
    }

} // namespace qkdtunnel
