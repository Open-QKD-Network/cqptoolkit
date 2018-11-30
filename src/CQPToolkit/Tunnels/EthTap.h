/*!
* @file
* @brief EthTun
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>

#include <cryptopp/secblock.h>
#include "CQPToolkit/Tunnels/DeviceIO.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Util/URI.h"
#include "CQPAlgorithms/Util/Strings.h"

namespace cqp
{

    namespace tunnels
    {
        /**
         * @brief The EthTap class
         * A data stream class which connects to an Ethernet tap/tun device
         * @see https://en.wikipedia.org/wiki/TUN/TAP
         */
        class CQPTOOLKIT_EXPORT EthTap : public DeviceIO, protected net::Socket
        {
        public:
            /// The mode for the device
            /// Tun devices don't include Ethernet headers
            enum class Mode { Tun, Tap};

            /// names of parameters in url
            struct Params
            {
                /// device name
                static NAMEDSTRING(name);
                /// netmask
                static NAMEDSTRING(netmask);
                /// mode valuefor tun devices
                static CONSTSTRING mode_tun = "tun";
                /// mode valuefor tun devices
                static CONSTSTRING mode_tap = "tap";
            };

            /**
             * @brief EthTap
             * Constructor
             * @param deviceName Optional device name
             * @param mode How the device will handle packets
             * @param address An ip address to assign to the device
             * @param netMask the mask to apply if an ip address is specified
             */
            EthTap(const std::string& deviceName, Mode mode, const std::string& address, const std::string& netMask);

            /**
             * @brief Create
             * Create an EthTap object
             * @param uri definition of a tap
             * @return A ready to use EthTap instance
             */
            static EthTap* Create(const URI& uri);

            /**
             * @brief GetName
             * @return The device name
             */
            const std::string& GetName() const
            {
                return name;
            }

            /**
             * @brief IsValid
             * @return true if the device has been created successfully
             */
            bool IsValid() const
            {
                return handle >= 0;
            }

            /**
             * @brief SetPersist
             * Set whether the device is kept once the last file handle is closed
             * @param on If true, the system will not delete the tun device once all handles are closed
             * @return true if the request succeeded
             */
            bool SetPersist(bool on);

            /**
             * @brief SetOwner
             * Change the owner or the device
             * @param user
             * @param group
             * @return
             */
            bool SetOwner(int user = -1, int group = -1);

            /**
             * @brief ~EthTap
             * Destructor
             */
            virtual ~EthTap() override;

            ///@{
            /// @name DeviceIO methods

            /// @copydoc DeviceIO::Read
            bool Read(void* data, size_t length, size_t& bytesReceived) override;
            /// @copydoc DeviceIO::Write
            bool Write(const void* data, size_t length) override;

            /// @}

            /// settings for a device
            struct DeviceDetails
            {
                /// the system name for the device
                std::string name;
                /// device details in uri form
                URI address;
                /// the kind of device
                Mode mode;
            };
            /**
             * @brief FindDevices
             * @return A list of devices
             */
            static std::vector<DeviceDetails> FindDevices();
        protected:
            /// The device name
            std::string name;
            /// available buffer
            size_t bufferSize = 0;

        };
    } // namespace tunnels
} // namespace cqp


