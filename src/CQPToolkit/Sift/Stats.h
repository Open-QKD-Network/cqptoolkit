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
#include "CQPToolkit/Statistics/Stat.h"
#include "CQPToolkit/Statistics/StatCollection.h"

namespace cqp
{
    namespace sift
    {

        /**
         * @brief The Statistics struct
         * The statistics reported by sifting
         */
        struct Statistics : stats::StatCollection
        {
            /// A group of values
            const char* parent = "Sifting";

            /// The number of bytes processed for this frame
            stats::Stat<size_t> bytesProduced {{parent, "Bytes Produced"}, stats::Units::Count};

            /// The total number of bytes processed by this instance
            stats::Stat<size_t> qubitsDisgarded {{parent, "Qubits Disgarded"}, stats::Units::Count};

            /// The time taken to compare qubit bases
            stats::Stat<double> comparisonTime {{parent, "Comparison Time"}, stats::Units::Count};
            /// The time taken to publish the results
            stats::Stat<double> publishTime {{parent, "Publish Time"}, stats::Units::Count};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                bytesProduced.Add(statsCb);
                qubitsDisgarded.Add(statsCb);
                comparisonTime.Add(statsCb);
                publishTime.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                bytesProduced.Remove(statsCb);
                qubitsDisgarded.Remove(statsCb);
                comparisonTime.Remove(statsCb);
                publishTime.Remove(statsCb);
            }

            /// @copydoc stats::StatCollection::AllStats
            std::vector<stats::StatBase*> AllStats() override
            {
                return
                {
                    &bytesProduced, &qubitsDisgarded, &comparisonTime, &publishTime
                };
            }
        }; // struct Statistics
    } // namespace sift
} // namespace cqp
