/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 5/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace ec
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by error correction
         */
        struct Stats : stats::StatCollection
        {
            /// The group to contain these stats
            static const constexpr char* parent = "ErrorCorrection";
            /// The number of errors corrected during this frame
            stats::Stat<double> Errors {{parent, "Errors"}, stats::Units::Percentage};

            /// The time took to transmit the qubits
            stats::Stat<size_t> TimeTaken {{parent, "TimeTaken"}, stats::Units::Milliseconds};

            /// The error rate of the quantum channel
            stats::Stat<double> QBER {{parent, "QBER"}, stats::Units::Percentage};

            /// @copydoc stats::StatCollection::Add
            void Add(stats::IAllStatsCallback* statsCb) override
            {
                Errors.Add(statsCb);
                TimeTaken.Add(statsCb);
                QBER.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            void Remove(stats::IAllStatsCallback* statsCb) override
            {
                Errors.Remove(statsCb);
                TimeTaken.Remove(statsCb);
                QBER.Remove(statsCb);
            }

        }; // struct Stats
    }// namespace ec
} // namespace cqp
