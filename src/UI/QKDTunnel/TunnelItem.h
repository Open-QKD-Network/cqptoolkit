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
#pragma once
#include <QStandardItem>
#include "Prototypes.h"

namespace qkdtunnel
{
    /**
     * @brief The TunnelItem class
     * Represent the tunnel endpoint in a gui
     */
    class TunnelItem : public QStandardItem
    {
    public:
        /**
         * @brief TunnelItem
         * Construct from basic info
         * @param name
         */
        TunnelItem(const QString& name = "TunnelEnd");

        /**
         * @brief TunnelItem
         * Constructor from a full set of details
         * @param details
         */
        TunnelItem(const cqp::remote::tunnels::Tunnel& details);

        /**
         * @brief SetDetails
         * Set the values from a complete set
         * @param details
         */
        void SetDetails(const cqp::remote::tunnels::Tunnel& details);

        /**
         * @brief GetDetails
         * @return the current values
         */
        cqp::remote::tunnels::Tunnel GetDetails() const;

        /**
         * @brief GetName
         * @return Tunnel name
         */
        std::string GetName() const;

        /**
         * @brief The Index enum
         * mapping from index to real data item
         */
        enum Index : int
        {
            name = 0,
            keyLifespanAge = Qt::UserRole + 1,
            keyLifespanAgeUnits,
            keyLifespanBytes,
            keyLifespanBytesUnits,
            remoteControllerIndex,
            remoteControllerUri,
            remoteControllerUuid,
            remoteControllerReferenceById,

            encryptionMethod_mode,
            encryptionMethod_subMode,
            encryptionMethod_blockCypher,
            encryptionMethod_keySizeBytes,

            startNode_clientDataPortUri,
            startNode_localChannelPort,
            startNode_channelUri,

            endNode_clientDataPortUri,
            endNode_localChannelPort,
            endNode_channelUri,

            active
        };

        /**
         * @brief isModified
         * @return true if modified
         */
        bool isModified() const
        {
            return modified;
        }
    protected:
        /// whether the values have been changed
        bool modified = false;
    };

} // namespace qkdtunnel
