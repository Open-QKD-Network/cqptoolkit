/*!
* @file
* @brief DeviceDialog
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DeviceDialog.h"
#include "CQPToolkit/Tunnels/RawSocket.h"
#include "CQPToolkit/Tunnels/EthTap.h"
#include "CQPToolkit/Util/Util.h"
#include "CQPToolkit/Datatypes/Tunnels.h"

namespace qkdtunnel
{

    DeviceDialog::DeviceDialog(QWidget* parent) :
        QDialog(parent)
    {
        setupUi(this);

    }

    void DeviceDialog::SetData(const QString& address)
    {
        using namespace cqp::tunnels;
        uri = address.toStdString();
        bool promisc = false;
        if(uri.GetFirstParameter(RawSocketParams::prom, promisc))
        {
            if(promisc)
            {
                dataPortProm->setCheckState(Qt::Checked);
            }
            else
            {
                dataPortProm->setCheckState(Qt::Unchecked);
            }
        }
        else
        {
            dataPortProm->setCheckState(Qt::PartiallyChecked);
        }

        std::string level;
        if(uri.GetFirstParameter(RawSocketParams::level, level))
        {
            dataPortCapture->setCurrentText(QString::fromStdString(level));
        }
        else
        {
            dataPortCapture->setCurrentIndex(0);
        }
        dataPortType->setCurrentText(QString::fromStdString(uri.GetScheme()));
        dataPortAddress->setText(QString::fromStdString(uri.GetHost()));
        dataPortPort->setValue(uri.GetPort());

        switch (cqp::str2int(uri.GetScheme().c_str()))
        {
        case cqp::str2int(DeviceTypes::eth):
            dataPortPort->setEnabled(false);
            deviceName->setEnabled(true);
            deviceNetmask->setEnabled(true);
            dataPortCapture->setEnabled(true);
            dataPortProm->setEnabled(true);
            deviceNetmask->setText(QString::fromStdString(uri[RawSocketParams::netmask]));
            deviceName->setText(QString::fromStdString(uri[RawSocketParams::name]));
            break;
        case cqp::str2int(DeviceTypes::tun):
        case cqp::str2int(DeviceTypes::tap):
            dataPortPort->setEnabled(false);
            deviceName->setEnabled(true);
            deviceNetmask->setEnabled(true);
            dataPortCapture->setEnabled(false);
            dataPortProm->setEnabled(false);
            deviceNetmask->setText(QString::fromStdString(uri[EthTap::Params::netmask]));
            deviceName->setText(QString::fromStdString(uri[EthTap::Params::name]));
            break;
        default:
            dataPortPort->setEnabled(true);
            deviceName->setEnabled(false);
            deviceNetmask->setEnabled(false);
            dataPortCapture->setEnabled(false);
            dataPortProm->setEnabled(false);
            deviceNetmask->setText("");
            deviceName->setText("");
            break;
        }
    }

    QString DeviceDialog::GetUri()
    {
        return QString::fromStdString(uri);
    }

    void DeviceDialog::on_deviceUri_editingFinished()
    {
        SetData(deviceUri->text());
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_dataPortAddress_editingFinished()
    {
        uri.SetHost(dataPortAddress->text().toStdString());
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_dataPortPort_valueChanged(int)
    {
        uri.SetPort(dataPortPort->value());
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_dataPortType_currentTextChanged(const QString &)
    {
        using namespace cqp::tunnels;
        uri.SetScheme(dataPortType->currentText().toStdString());

        switch (cqp::str2int(uri.GetScheme().c_str()))
        {
        case cqp::str2int(DeviceTypes::eth):
            dataPortPort->setEnabled(false);
            uri.SetPort(0);
            deviceName->setEnabled(true);
            deviceNetmask->setEnabled(true);
            dataPortCapture->setEnabled(true);
            dataPortProm->setEnabled(true);
            break;
        case cqp::str2int(DeviceTypes::tun):
        case cqp::str2int(DeviceTypes::tap):
            dataPortPort->setEnabled(false);
            uri.SetPort(0);
            deviceName->setEnabled(true);
            deviceNetmask->setEnabled(true);
            dataPortCapture->setEnabled(false);
            uri.RemoveParameter(RawSocketParams::level);
            dataPortProm->setEnabled(false);
            uri.RemoveParameter(RawSocketParams::prom);
            break;
        default:
            dataPortPort->setEnabled(true);
            deviceName->setEnabled(false);
            uri.RemoveParameter(EthTap::Params::name);
            deviceNetmask->setEnabled(false);
            uri.RemoveParameter(EthTap::Params::netmask);
            dataPortCapture->setEnabled(false);
            uri.RemoveParameter(RawSocketParams::level);
            dataPortProm->setEnabled(false);
            uri.RemoveParameter(RawSocketParams::prom);
            break;
        }
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_dataPortCapture_currentTextChanged(const QString &)
    {
        using namespace cqp::tunnels;
        if(dataPortCapture->currentIndex() > 0)
        {
            uri.SetParameter(RawSocketParams::level, dataPortCapture->currentText().toStdString());
        }
        else
        {
            uri.RemoveParameter(RawSocketParams::level);
        }
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_dataPortProm_stateChanged(int arg1)
    {
        using namespace cqp::tunnels;
        switch (arg1)
        {
        case Qt::Unchecked:
            uri.SetParameter(RawSocketParams::prom, "false");
            break;
        case Qt::PartiallyChecked:
            uri.RemoveParameter(RawSocketParams::prom);
            break;
        case Qt::Checked:
            uri.SetParameter(RawSocketParams::prom, "true");
            break;
        }
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_deviceName_editingFinished()
    {
        using namespace cqp::tunnels;
        const std::string value = deviceName->text().toStdString();
        if(value.empty())
        {
            uri.RemoveParameter(RawSocketParams::name);
        } // if
        else
        {
            switch (cqp::str2int(uri.GetScheme().c_str()))
            {
            case cqp::str2int(DeviceTypes::eth):
                uri.SetParameter(RawSocketParams::name, value);
                break;
            case cqp::str2int(DeviceTypes::tap):
            case cqp::str2int(DeviceTypes::tun):
                uri.SetParameter(EthTap::Params::name, value);
                break;
            } // switch
        } // else
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

    void DeviceDialog::on_deviceNetmask_editingFinished()
    {
        using namespace cqp::tunnels;
        const std::string value = deviceNetmask->text().toStdString();
        if(value.empty())
        {
            uri.RemoveParameter(RawSocketParams::netmask);
        } // if
        else
        {
            switch (cqp::str2int(uri.GetScheme().c_str()))
            {
            case cqp::str2int(DeviceTypes::eth):
                uri.SetParameter(RawSocketParams::netmask, value);
                break;
            case cqp::str2int(DeviceTypes::tap):
            case cqp::str2int(DeviceTypes::tun):
                uri.SetParameter(EthTap::Params::netmask, value);
                break;
            } // switch
        } // else
        deviceUri->setText(QString::fromStdString(uri.ToString()));
    }

} // namespace qkdtunnel
