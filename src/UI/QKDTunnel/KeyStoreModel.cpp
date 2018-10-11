/*!
* @file
* @brief KeyStoreModel
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "KeyStoreModel.h"

namespace qkdtunnel
{

    KeyStoreModel::KeyStoreModel()
    {

    }

    void KeyStoreModel::AppendRow(const std::string& name, const std::string& connectionAddress, const std::string& id)
    {
        QStandardItem* newItem = new QStandardItem();
        newItem->setData(QString::fromStdString(name), Index::name);
        newItem->setData(QString::fromStdString(connectionAddress), Index::connectionAddress);
        newItem->setData(QString::fromStdString(id), Index::id);
        appendRow(newItem);
    }

} // namespace qkdtunnel
