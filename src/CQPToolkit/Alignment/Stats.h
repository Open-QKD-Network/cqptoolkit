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
#include "CQPAlgorithms/Statistics/Stat.h"
#include "CQPAlgorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace align
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by alignment
         */
        struct Statistics : stats::StatCollection
        {
            /// A group of values
            const char* parent = "Alignment";

            /// The number of errors begin corrected per frame on average
            stats::Stat<double> overhead {{parent, "Overhead"}, stats::Units::Percentage};

            /// The time took to transmit the qubits
            stats::Stat<double> timeTaken {{parent, "TimeTaken"}, stats::Units::Milliseconds};

            /// The total number of bytes processed by this instance
            stats::Stat<size_t> qubitsProcessed {{parent, "QubitsProcessed"}, stats::Units::Count};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                overhead.Add(statsCb);
                timeTaken.Add(statsCb);
                qubitsProcessed.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                overhead.Remove(statsCb);
                timeTaken.Remove(statsCb);
                qubitsProcessed.Remove(statsCb);
            }

            /// @copydoc stats::StatCollection::AllStats
            std::vector<stats::StatBase*> AllStats() override
            {
                return
                {
                    &overhead, &timeTaken, &qubitsProcessed
                };
            }
        };
    } // namespace stats
} // namespace cqp
