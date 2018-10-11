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
#pragma once

#include <QStandardItem>

namespace qkdtunnel
{

    /**
     * @brief The KeyStoreModel class
     */
    class KeyStoreModel : public QStandardItemModel
    {
    public:
        /// mapping of index to data item
        enum Index
        {
            name = 0,
            connectionAddress = Qt::UserRole + 1,
            id,
            _Last
        };

        /// Contructor
        KeyStoreModel();

        /**
         * @brief AppendRow
         * @param name
         * @param connectionAddress
         * @param id
         */
        void AppendRow(const std::string& name, const std::string& connectionAddress, const std::string& id);
    };

} // namespace qkdtunnel


