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

#include <array>
#include <condition_variable>
#include <cryptopp/secblock.h>
#include "Algorithms/Util/Process.h"
#include "Algorithms/Datatypes/Base.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

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
        /// Destructor
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

        /**
         * @brief WaitForKey
         * Blocks until the device produces key or an event forces the method to return
         * @return true if key is available
         */
        bool WaitForKey();

        /**
         * @brief The Statistics struct
         * The statistics reported by Gating
         */
        struct Stats : stats::StatCollection
        {
            /// The group to contain these stats
            const char* parent = "Key";
            /// detector efficiency
            stats::Stat<double> Visibility {{parent, "Visibility"}, stats::Units::Percentage, "A measurement of the detectors ability"};

            /// bit error rate
            stats::Stat<double> Qber {{parent, "QBER"}, stats::Units::Count, "Quantum Bit Error Rate"};

            /// key bits produced
            stats::Stat<size_t> keySize {{parent, "Key size"}, stats::Units::Count, "Bits produced"};

            /// key bits produced
            stats::Stat<double> keyRate {{parent, "Key Rate"}, stats::Units::Percentage, "Bits/Second key generated"};

            /// key bits produced
            stats::Stat<size_t> lineLength {{parent, "Line length"}, stats::Units::Hz, "Number of meters of fibre/space between alice and bob"};

            /// @copydoc stats::StatCollection::Add
            void Add(stats::IAllStatsCallback* statsCb) override
            {
                Visibility.Add(statsCb);
                Qber.Add(statsCb);
                keySize.Add(statsCb);
                keyRate.Add(statsCb);
                lineLength.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            void Remove(stats::IAllStatsCallback* statsCb) override
            {
                Visibility.Remove(statsCb);
                Qber.Remove(statsCb);
                keySize.Remove(statsCb);
                keyRate.Remove(statsCb);
                lineLength.Remove(statsCb);
            }

            /// @copydoc stats::StatCollection::AllStats
            std::vector<stats::StatBase*> AllStats() override
            {
                return
                {
                    &Visibility, &Qber, &keySize, &keyRate, &lineLength
                };
            }
        }
        /// Provides access to the stats generated by this class
        stats;

    protected:
        /// Start the IDQ driver process
        /// @param args arguments to pass to the process
        void LaunchProc(const std::vector<std::string>& args);
        /// Generate a pre-shared key and store it in the required location for the IDQ driver
        /// @param psk pre-shared key for the device pair
        /// @returns true on success
        bool CreateInitialPsk(const DataBlock& psk);

        /// Is this device alice
        bool alice = false;
        /// Thread for managing the process
        std::thread procHandler;
        /// ID of the child process
        Process proc;
        /// The program name to launch
        static const std::string ProgramName;
        /// allows a caller to wait for key to arrive
        std::mutex keyReadyMutex;
        /// allows a caller to wait for key to arrive
        std::condition_variable keyReadyCv;
        /// flag for detecting when key has arrived
        bool keyAvailable = false;
        /// allows locked threads to exit gracefully
        bool shutdown = false;
    };

}
