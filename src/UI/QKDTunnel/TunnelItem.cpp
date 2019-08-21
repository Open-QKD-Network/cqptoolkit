/*!
* @file
* @brief TunnelItem
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/10/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TunnelItem.h"
#include <QUrl>
#include "Algorithms/Datatypes/UUID.h"
#include "QKDInterfaces/Tunnels.pb.h"
#include <QTime>
#include "Algorithms/Logging/Logger.h"

namespace qkdtunnel
{

    TunnelItem::TunnelItem(const QString& name)
    {
        setData(name, Index::name);
        setData(-1, Index::remoteControllerIndex);
        setData(true, Index::remoteControllerReferenceById);
        setData("", Index::remoteControllerUri);
        setData("", Index::remoteControllerUuid);
        setData("GCM", Index::encryptionMethod_mode);
        setData("Tables2K", Index::encryptionMethod_subMode);
        setData("AES", Index::encryptionMethod_blockCypher);
        setData(16, Index::encryptionMethod_keySizeBytes);

        setIcon(QIcon(":/icons/tun"));
    }

    TunnelItem::TunnelItem(const cqp::remote::tunnels::Tunnel& details)
    {
        SetDetails(details);
    }

    void TunnelItem::SetDetails(const cqp::remote::tunnels::Tunnel& details)
    {
        using namespace cqp;
        setIcon(QIcon(":/icons/tun"));
        setData(QString::fromStdString(details.name()), name);

        using cqp::remote::Duration;
        switch (details.keylifespan().maxage().scale_case())
        {
        case Duration::ScaleCase::kSeconds:
            setData(QTime(0, 0, details.keylifespan().maxage().seconds(), 0), Index::keyLifespanAge);
            break;
        case Duration::ScaleCase::kMilliseconds:
            setData(QTime(0, 0, 0, details.keylifespan().maxage().milliseconds()), Index::keyLifespanAge);
            break;
        case Duration::ScaleCase::SCALE_NOT_SET:
            break;
        }
        setData(static_cast<qulonglong>(details.keylifespan().maxbytes()), Index::keyLifespanBytes);
        setData(QString::fromStdString(""), Index::remoteControllerUri);
        setData(QString::fromStdString(""), Index::remoteControllerUuid);
        if(!details.remotecontrolleruri().empty())
        {
            setData(QString::fromStdString(details.remotecontrolleruri()), Index::remoteControllerUri);
            setData(false, Index::remoteControllerReferenceById);
        }
        else if(!details.remotecontrolleruuid().empty())
        {
            setData(QString::fromStdString(details.remotecontrolleruuid()), Index::remoteControllerUuid);
            setData(true, Index::remoteControllerReferenceById);
        }
        else
        {
            LOGERROR("No remote controller defined");
        }
        setData(QString::fromStdString(details.remotecontrolleruuid()), Index::remoteControllerUuid);

        setData(QString::fromStdString(details.encryptionmethod().mode()), Index::encryptionMethod_mode);
        setData(QString::fromStdString(details.encryptionmethod().submode()), Index::encryptionMethod_subMode);
        setData(QString::fromStdString(details.encryptionmethod().blockcypher()), Index::encryptionMethod_blockCypher);
        setData(details.encryptionmethod().keysizebytes(), Index::encryptionMethod_keySizeBytes);

        setData(QString::fromStdString(details.startnode().clientdataporturi()), Index::startNode_clientDataPortUri);

        setData(QString::fromStdString(details.endnode().clientdataporturi()), Index::endNode_clientDataPortUri);

    } // SetDetails

    cqp::remote::tunnels::Tunnel TunnelItem::GetDetails() const
    {
        using namespace cqp;
        using namespace std::chrono;
        cqp::remote::tunnels::Tunnel result;
        result.set_name(data(Index::name).toString().toStdString());

        if(data(Index::keyLifespanAge).toUInt() > 0)
        {
            duration<uint64_t> timeVal {};
            switch (data(Index::keyLifespanAgeUnits).toInt())
            {
            case 0: // seconds
                timeVal = seconds(data(Index::keyLifespanAge).toUInt());
                break;
            case 1: // minutes
                timeVal = duration_cast<duration<uint64_t>>( minutes(data(Index::keyLifespanAge).toUInt()) );
                break;
            case 2: // hours
                timeVal = duration_cast<duration<uint64_t>>( hours(data(Index::keyLifespanAge).toUInt()) );
                break;
            }
            result.mutable_keylifespan()->mutable_maxage()->set_seconds(timeVal.count());
        }

        if(data(Index::keyLifespanBytes).toUInt() > 0)
        {
            uint64_t bytes = data(Index::keyLifespanBytes).toUInt() * 1024;
            for(int count = 0; count < data(Index::keyLifespanBytesUnits).toInt(); count++)
            {
                bytes *= 1024;
            }
            result.mutable_keylifespan()->set_maxbytes(bytes);
        }

        if(data(Index::remoteControllerReferenceById).toBool())
        {
            result.set_remotecontrolleruuid(data(Index::remoteControllerUuid).toString().toStdString());
        }
        else
        {
            result.set_remotecontrolleruri(data(Index::remoteControllerUri).toString().toStdString());
        }


        result.mutable_encryptionmethod()->set_mode(data(Index::encryptionMethod_mode).toString().toStdString());
        result.mutable_encryptionmethod()->set_submode(data(Index::encryptionMethod_subMode).toString().toStdString());
        result.mutable_encryptionmethod()->set_blockcypher(data(Index::encryptionMethod_blockCypher).toString().toStdString());
        result.mutable_encryptionmethod()->set_keysizebytes(data(Index::encryptionMethod_keySizeBytes).toUInt());

        result.mutable_startnode()->set_clientdataporturi(data(Index::startNode_clientDataPortUri).toString().toStdString());

        result.mutable_endnode()->set_clientdataporturi(data(Index::endNode_localChannelPort).toString().toStdString());

        return result;
    }

    std::string TunnelItem::GetName() const
    {
        return data(Index::name).toString().toStdString();
    } // GetDetails

} // namespace qkdtunnel
