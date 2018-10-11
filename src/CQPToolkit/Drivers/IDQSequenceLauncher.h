/*!
* @file
* @brief IDQSequenceLauncher
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/3/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>
#include <thread>
#include "CQPToolkit/Util/Platform.h"
#include <array>
#include <cryptopp/secblock.h>
#include "CQPToolkit/Util/Process.h"
#include "CQPToolkit/Datatypes/Base.h"

namespace cqp
{
    /// Start the IDQ driver program for communicating with the Clavis 2
    class CQPTOOLKIT_EXPORT IDQSequenceLauncher
    {
    public:
        /// Size of the PSK used with the clavis
        const static constexpr int PreSharedKeyLength = 32;

        /// Launch the IDQ program in Alice mode
        /// @param[in] initialPsk Pre shared key to authenticate the two devices
        /// @param[in] otherUnit Address of the paired device
        /// @param[in] lineAttenuation in db
        IDQSequenceLauncher(const DataBlock& initialPsk, const std::string& otherUnit, double lineAttenuation);
        /// Distructor
        virtual ~IDQSequenceLauncher();

        /// USB vendor ID for claivs 2 devices
        static const uint16_t IDQVendorId = 0x1DDC;
        /// USB product ID for claivs 2 devices - Alice
        static const uint16_t Clavis2ProductIDAlice = 0x0203;
        /// USB product ID for claivs 2 devices - Bob
        static const uint16_t Clavis2ProductIDBob = 0x0204;

        /// url scheme for this device
        static CONSTSTRING ClavisScheme = "clavis";

        /**
         * @brief DeviceIsAlice
         * @return true if the device is alice
         */
        bool DeviceIsAlice() const;

        /// Which kind/end the device is
        enum class DeviceType { None, Alice, Bob };
        /// Returns true is any clavis device was found
        /// @returns true on success
        static DeviceType DeviceFound();

        /// Returns true is the speified product ID is attached to the machine
        /// @param[in] devId USB product ID of the device to find.
        /// @returns true on success
        static bool DeviceFound(uint16_t devId);

        /**
         * @brief Running
         * @return true if the process is running
         */
        bool Running();
    protected:
        /// Start the IDQ driver process
        /// @param args arguments to pass to the process
        void LaunchProc(const std::vector<std::string>& args);
        /// Generate a pre-shared key and store it in the required location for the IDQ driver
        /// @param psk preshared key for the device pair
        /// @returns true on success
        bool CreateInitialPsk(const DataBlock& psk);

        /// Is this device alice
        bool alice = false;
        /// Thread for managing the process
        std::thread procHandler;
        /// ID of the child process
        Process proc;
    };

}
