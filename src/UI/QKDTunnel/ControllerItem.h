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
#pragma once
#include <QModelIndex>
#include <QDateTime>
#include <QStandardItem>
#include <QStringListModel>
#include "TunnelItem.h"

namespace qkdtunnel
{
    class DeviceItem;
    class DynEndpointItem;

    /**
     * @brief The ControllerItem class
     * Represents the controller as a list item
     */
    class ControllerItem : public QStandardItem
    {
    public:
        /// possible roles for the get/set data to index into the stored data
        enum Index : int
        {
            name = 0,
            connectionAddress = Qt::UserRole + 1,
            listenAddress,
            listenPort,
            Id,
            localKeyFactoryById,
            localKeyFactoryIndex,
            localKeyFactoryId,
            localKeyFactoryUri,
            credUseTls,
            credCertFile,
            credKeyFile,
            credCaFile,
            running,
            lastUpdated,
            _Last
        };

        /**
         * @brief ControllerItem
         * Default constructor
         * @param id Unique id for this item
         * @param name User readable name
         */
        ControllerItem(const cqp::UUID& id, const QString& name = "NewController");

        /**
         * @brief ControllerItem
         * Construct from settings
         * @param details values to initialise with
         */
        explicit ControllerItem(const cqp::remote::tunnels::ControllerDetails& details);

        /**
         * @brief GetId
         * @return The unique id for this controller
         */
        std::string GetId() const;

        /**
         * @brief GetURI
         * @return URI of the controller
         */
        cqp::URI GetURI() const;

        /**
         * @brief GetName
         * @return user readable name for the controller
         */
        QString GetName() const;

        /**
         * @brief Submit
         * commit the changes to the running controller instance
         * @return true if the changes were excepted.
         */
        bool Submit();

        /**
         * @brief Revert
         * reset the changes to the current controller state
         * @return true if the settings were retrieved.
         */
        bool Revert();

        /**
         * @brief SetIsRunning
         * Record whether the controller is running
         * @todo Change this to actually start/stop the service
         * @param active
         */
        void SetIsRunning(bool active);

        /**
         * @brief GetTunnels
         * @return All defined tunnels
         */
        QList<TunnelItem*> GetTunnels() const;

        /**
         * @brief GetTunnel
         * @param name Which tunnel to get
         * @return The tunnel details or nullptr if no tunnel is defined
         */
        TunnelItem* GetTunnel(const std::string& name);

        /**
         * @brief GetCryptoModes
         * @return The choices for the user
         */
        QAbstractItemModel* GetCryptoModes()
        {
            return &supportedModes;
        }

        /**
         * @brief GetCryptoSubModes
         * @return The choices for the user
         */
        QAbstractItemModel* GetCryptoSubModes()
        {
            return &supportedSubModes;
        }

        /**
         * @brief GetCryptoBlockCyphers
         * @return The choices for the user
         */
        QAbstractItemModel* GetCryptoBlockCyphers()
        {
            return &supportedBlockCyphers;
        }

        /**
         * @brief GetCryptoKeySizes
         * @return The choices for the user
         */
        QAbstractItemModel* GetCryptoKeySizes()
        {
            return &supportedKeySizes;
        }

        /**
         * @brief GetDetails
         * @return The current values for this item
         */
        cqp::remote::tunnels::ControllerDetails GetDetails() const;

        /**
         * @brief SetDetails
         * Change the settings
         * @param details The new settings
         */
        void SetDetails(const cqp::remote::tunnels::ControllerDetails& details);

        /**
         * @brief isModified
         * @return true if there are uncommitted changes
         */
        bool isModified() const;

        ///@{
        /// @name QStandardItem interface

        /**
         * @brief setData
         * Change the data
         * @param value new value
         * @param role index to change
         */
        void setData(const QVariant& value, int role) override;
        ///@}
    protected:

        /// Has the data changed since the last time it was committed
        bool modified = false;

        /// list of choices for the user
        QStringListModel supportedModes;
        /// list of choices for the user
        QStringListModel supportedSubModes;
        /// list of choices for the user
        QStringListModel supportedBlockCyphers;
        /// list of choices for the user
        QStringListModel supportedKeySizes;

    }; // class ControllerItem


} // namespace qkdtunnel
