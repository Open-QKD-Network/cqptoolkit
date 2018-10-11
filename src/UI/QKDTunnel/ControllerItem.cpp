/*!
* @file
* @brief ControllerItem
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ControllerItem.h"
#include "CQPToolkit/Util/UUID.h"
#include "CQPToolkit/Util/URI.h"
#include <QUrl>
#include "QKDInterfaces/Tunnels.pb.h"

namespace qkdtunnel
{

#if !defined(DOXYGEN)
    const QStringList DefaultModes {"GCM" };
    const QStringList DefaultSubModes {"Tables2K", "Tables64K"};
    const QStringList DefaultBlockCyphers {"AES"};
    const QStringList DefaultKeySizes {"16", "32"};
#endif

    ControllerItem::ControllerItem(const cqp::UUID& id, const QString& name)
    {
        // setup some default values for display
        setText(name);
        setIcon(QIcon(":/icons/Controller"));

        setData(QString::fromStdString(id), Index::Id);
        setData(QDateTime::currentDateTime(), Index::lastUpdated);
        setData(-1, Index::localKeyFactoryIndex);
        setData(true, Index::localKeyFactoryById);
        setData("", Index::localKeyFactoryId);
        setData("", Index::localKeyFactoryUri);

        supportedModes.setStringList(DefaultModes);
        supportedSubModes.setStringList(DefaultSubModes);
        supportedBlockCyphers.setStringList(DefaultBlockCyphers);
        supportedKeySizes.setStringList(DefaultKeySizes);

    }

    ControllerItem::ControllerItem(const cqp::remote::tunnels::ControllerDetails& details)
    {

        SetDetails(details);
    }

    std::string ControllerItem::GetId() const
    {
        return data(Index::Id).toString().toStdString();
    }

    cqp::URI ControllerItem::GetURI() const
    {
        return cqp::URI(data(Index::connectionAddress).toString().toStdString());
    }

    QString ControllerItem::GetName() const
    {
        return data(Index::name).toString();
    }

    QList<TunnelItem*> ControllerItem::GetTunnels() const
    {
        QList<TunnelItem*> result;

        for(int childIndex = 0 ; childIndex < this->rowCount(); childIndex++)
        {
            TunnelItem* tun = dynamic_cast<TunnelItem*>(child(childIndex));
            if(tun)
            {
                result << tun;
            }
        }
        return result;
    }

    TunnelItem* ControllerItem::GetTunnel(const std::string& name)
    {
        TunnelItem* result = nullptr;
        for(int row = 0; row < rowCount(); row++)
        {
            result = dynamic_cast<TunnelItem*>(child(row));
            if(result && result->GetName() == name)
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

    void ControllerItem::SetDetails(const cqp::remote::tunnels::ControllerDetails& details)
    {
        using cqp::remote::tunnels::ControllerDetails;
        setIcon(QIcon(":/icons/controller"));
        setData(QString::fromStdString(details.name()), Index::name);
        setData(QString::fromStdString(details.id()), Index::Id);
        setData(QString::fromStdString(details.listenaddress()), Index::listenAddress);
        setData(details.listenport(), Index::listenPort);
        setData(QDateTime::currentDateTime(), Index::lastUpdated);
        setData(QUrl(details.connectionuri().c_str()), Index::connectionAddress);

        setData(QString::fromStdString(""), Index::localKeyFactoryUri);
        setData(QString::fromStdString(""), Index::localKeyFactoryId);
        switch (details.localKeyFactory_case())
        {
        case ControllerDetails::LocalKeyFactoryCase::kLocalKeyFactoryUri:
            setData(QString::fromStdString(details.localkeyfactoryuri()), Index::localKeyFactoryUri);
            setData(false, Index::localKeyFactoryById);
            break;
        case ControllerDetails::LocalKeyFactoryCase::kLocalKeyFactoryUuid:
            setData(QString::fromStdString(details.localkeyfactoryuuid()), Index::localKeyFactoryId);
            setData(true, Index::localKeyFactoryById);
            break;
        case ControllerDetails::LocalKeyFactoryCase::LOCALKEYFACTORY_NOT_SET:
            break;
        }

        for(auto& tunDetails : details.tunnels())
        {
            TunnelItem* tunnel = GetTunnel(tunDetails.first);
            if(tunnel)
            {
                tunnel->SetDetails(tunDetails.second);
            }
            else
            {
                appendRow(new TunnelItem(tunDetails.second));
            }
        }
        setData(details.credentials().usetls(), Index::credUseTls);
        setData(QString::fromStdString(details.credentials().certchainfile()), Index::credCertFile);
        setData(QString::fromStdString(details.credentials().privatekeyfile()), Index::credKeyFile);
        setData(QString::fromStdString(details.credentials().rootcertsfile()), Index::credCaFile);

        modified = false;
    }

    cqp::remote::tunnels::ControllerDetails ControllerItem::GetDetails() const
    {
        cqp::remote::tunnels::ControllerDetails result;
        result.set_name(data(Index::name).toString().toStdString());
        result.set_id(data(Index::Id).toString().toStdString());
        result.set_listenaddress(data(Index::listenAddress).toString().toStdString());
        result.set_listenport(static_cast<uint16_t>(data(Index::listenPort).toUInt()));
        result.set_connectionuri(data(Index::connectionAddress).toString().toStdString());

        if(data(Index::localKeyFactoryById).toBool())
        {
            result.set_localkeyfactoryuuid(data(Index::localKeyFactoryId).toString().toStdString());
        }
        else
        {
            result.set_localkeyfactoryuri(data(Index::localKeyFactoryUri).toString().toStdString());
        }

        for(auto tun : GetTunnels())
        {
            if(tun)
            {
                result.mutable_tunnels()->insert({tun->GetName(), tun->GetDetails()});
            }
        }
        result.mutable_credentials()->set_usetls(data(Index::credUseTls).toBool());
        result.mutable_credentials()->set_certchainfile(data(Index::credCertFile).toString().toStdString());
        result.mutable_credentials()->set_privatekeyfile(data(Index::credKeyFile).toString().toStdString());
        result.mutable_credentials()->set_rootcertsfile(data(Index::credCaFile).toString().toStdString());

        return result;
    }

    bool ControllerItem::isModified() const
    {
        bool result = modified;
        if(!result)
        {
            for(int row = 0; row < rowCount(); row++)
            {
                TunnelItem* tunnel = dynamic_cast<TunnelItem*>(child(row));
                if(tunnel && tunnel->isModified())
                {
                    result = true;
                    break; // for
                }
            }
        }

        return result;
    }

    bool ControllerItem::Submit()
    {
        bool result = false; // = ep != nullptr;
        /*if(ep)
        {
            if(modified)
            {
                // pass the changes to the controller
                cqp::tunnels::ControllerDetails details = GetDetails();
                result = ep->ModifySettings(details);
                // update with corrected values from the server
                SetDetails(details);
                modified = !result;
            }
        }*/
        return result;
    }

    bool ControllerItem::Revert()
    {
        bool result = false; // = ep != nullptr;
        /*if(ep)
        {
            // get the settings from the contorller
            cqp::tunnels::ControllerDetails details;
            result = ep->GetSettings(details);
            SetDetails(details);
            modified = false;
        }*/
        return result;
    }

    void ControllerItem::SetIsRunning(bool active)
    {
        setData(active, Index::running);
        setData(QDateTime::currentDateTime(), Index::lastUpdated);
    }

    void ControllerItem::setData(const QVariant& value, int role)
    {
        modified = true;
        QStandardItem::setData(value, role);
    }

} // namespace qkdtunnel

