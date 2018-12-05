/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace keygen
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by key generation
         */
        struct Statistics : public stats::StatCollection
        {

            /// A group of values
            const char* parent = {"Key Generation"};

            /// The number of keys added
            stats::Stat<size_t> unusedKeysAvailable {{parent, "Unused Keys Available"}, stats::Units::Count};

            /// The number of keys added
            stats::Stat<size_t> reservedKeys {{parent, "Reserved Keys"}, stats::Units::Count};

            /// The number of keys added
            stats::Stat<size_t> keyGenerated {{parent, "Key Generated"}, stats::Units::Count};

            /// The number of keys added
            stats::Stat<size_t> keyUsed {{parent, "Key Used"}, stats::Units::Count};

            /// @copydoc stats::StatCollection::Add
            void Add(stats::IAllStatsCallback* statsCb)
            {
                unusedKeysAvailable.Add(statsCb);
                reservedKeys.Add(statsCb);
                keyGenerated.Add(statsCb);
                keyUsed.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            void Remove(stats::IAllStatsCallback* statsCb)
            {
                unusedKeysAvailable.Remove(statsCb);
                reservedKeys.Remove(statsCb);
                keyGenerated.Remove(statsCb);
                keyUsed.Remove(statsCb);
            }

            /**
             * @copydoc stats::StatCollection::AllStats
             */
            std::vector<stats::StatBase*> AllStats()
            {
                return
                {
                    &unusedKeysAvailable, &reservedKeys, &keyGenerated, &keyUsed
                };
            }
        }; // struct Statistics
    } // namespace keygen
} // namespace cqp
