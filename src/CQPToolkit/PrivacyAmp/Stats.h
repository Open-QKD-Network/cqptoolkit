/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace privacy
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by privacy amplification
         */
        struct Statistics : stats::StatCollection
        {
            /// A group of values
            const char* parent = "Privacy Amplification";

            /// The total number of bytes used during PA for this frame
            stats::Stat<size_t> bytesDiscarded {{parent, "BytesDiscarded"}, stats::Units::Count};
            /// The total number of bytes used during PA for this frame
            stats::Stat<size_t> keysEmitted {{parent, "KeyEmitted"}, stats::Units::Count};

            /// The time took to transmit the qubits
            stats::Stat<double> timeTaken {{parent, "TimeTaken"}, stats::Units::Milliseconds};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                bytesDiscarded.Add(statsCb);
                keysEmitted.Add(statsCb);
                timeTaken.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                bytesDiscarded.Remove(statsCb);
                keysEmitted.Remove(statsCb);
                timeTaken.Remove(statsCb);
            }

        }; // namespace Stats
    } // namespace privacy
} // namespace cqp
