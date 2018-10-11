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
#include "CQPToolkit/Statistics/Stat.h"
#include "CQPToolkit/Statistics/StatCollection.h"

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

            /// @copydoc stats::StatCollection::AllStats
            std::vector<stats::StatBase*> AllStats() override
            {
                return
                {
                    &TimeOpen
                };
            }
        }; // struct Statistics
    } // namespace keys

} // namespace cqp
