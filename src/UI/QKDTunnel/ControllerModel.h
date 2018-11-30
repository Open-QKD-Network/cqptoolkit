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
#ifndef CONTROLLERMODEL_H
#define CONTROLLERMODEL_H

#include "CQPAlgorithms/Datatypes/Services.h"
#include "ControllerItem.h"
#include "CQPAlgorithms/Datatypes/Base.h"
#include <QStandardItem>
#include "QKDInterfaces/Tunnels.pb.h"

Q_DECLARE_METATYPE(QStandardItem*)

namespace qkdtunnel
{
    /**
     * @brief The ControllerModel class
     * Stores a list of controllers
     */
    class ControllerModel : public QStandardItemModel
    {
        Q_OBJECT

    public:
        /**
         * @brief ControllerModel
         * constructor
         * @param parent
         */
        explicit ControllerModel(QObject *parent = nullptr);

        ///@{
        /// @name QAbstractItemModel interface

        /**
         * @brief submit
         * @return true on success
         */
        bool submit() override;
        /**
         * @brief revert
         */
        void revert() override;
        ///@}

        /**
         * @brief SetRemote
         * Set values from a remote controller
         * @param services
         */
        void SetRemote(const cqp::RemoteHost& host);

        /**
         * @brief SetRemote
         * Set values from a remote controller
         * @param services
         */
        void SetRemote(const cqp::RemoteHosts& services);

        /**
         * @brief RemoveController
         * @param id
         * @return true on success
         */
        bool RemoveController(const std::string& id);

        /**
         * Template function to walk the items tree and retrieve
         * the first parent of type T found
         * @tparam T The type of class to look for
         * @param index The start point
         * @return The object of type T if found or nullptr
         */
        template<class T>
        T* FindItem(const QModelIndex& index)
        {
            QModelIndex lookat = index;
            T* result = dynamic_cast<T*>(itemFromIndex(lookat));

            while(result == nullptr && lookat.isValid())
            {
                result = dynamic_cast<T*>(itemFromIndex(lookat));
                lookat = lookat.parent();
            }
            return result;
        }

        /**
         * Template function to walk the items tree and retrieve
         * the first parent of type T found
         * @tparam T The type of class to look for
         * @param item The start point
         * @return The object of type T if found or nullptr
         */
        template<class T>
        T* FindItem(QStandardItem* item)
        {
            QStandardItem* lookat = item;
            T* result = dynamic_cast<T*>(lookat);

            while(result == nullptr && lookat != nullptr)
            {
                result = dynamic_cast<T*>(lookat);
                lookat = lookat->parent();
            }
            return result;
        }

        /**
         * @brief GetController
         * @param id
         * @return the controller which this item represents
         */
        ControllerItem* GetController(const std::string& id) const;
    protected:
    }; // class ControllerModel

} // namespace qkdtunnel

#endif // CONTROLLERMODEL_H
