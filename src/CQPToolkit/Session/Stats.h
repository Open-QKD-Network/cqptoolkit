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
    namespace session
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by privacy amplification
         */
        struct Statistics : stats::StatCollection
        {

            /// A group of values
            const char* parent = "Session";

            /// The number of bytes processed for this frame
            stats::Stat<double> TimeOpen {{parent, "Time Open"}, stats::Units::Milliseconds};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                TimeOpen.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                TimeOpen.Remove(statsCb);
            }

        }; // struct Statistics
    } // namespace keys

} // namespace cqp
