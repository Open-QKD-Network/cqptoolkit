/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 12/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace tunnels
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by tunnels
         */
        struct Statistics : stats::StatCollection
        {
            /// A group of values
            const char* parent = "Tunnels";

            /// The number of bytes processed for this message
            stats::Stat<size_t> bytesEncrypted {{parent, "Bytes Encrypted"}, stats::Units::Count};

            /// The time taken to encrypt a message
            stats::Stat<double> encryptTime {{parent, "Encryption Time"}, stats::Units::Count};
            /// The time taken to encrypt a message
            stats::Stat<double> decryptTime {{parent, "Decryption Time"}, stats::Units::Count};
            /// The time taken to change the encryption key
            stats::Stat<double> keyChangeTime {{parent, "Key Change Time"}, stats::Units::Count};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                bytesEncrypted.Add(statsCb);
                encryptTime.Add(statsCb);
                decryptTime.Add(statsCb);
                keyChangeTime.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                bytesEncrypted.Remove(statsCb);
                encryptTime.Remove(statsCb);
                decryptTime.Remove(statsCb);
                keyChangeTime.Remove(statsCb);
            }

        }; // struct Statistics
    } // namespace tunnels
} // namespace cqp
