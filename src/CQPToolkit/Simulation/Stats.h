/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/Stat.h"
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{
    namespace sim
    {
        /**
         * @brief The Statistics struct
         * The statistics reported by simulation
         */
        struct Statistics : stats::StatCollection
        {
            /// A group of values
            const char* parent = "TransmitterGroup";

            /// The total number of bytes processed by this instance
            stats::Stat<size_t> qubitsTransmitted {{parent, "Qubits Transmitted"}, stats::Units::Count};

            /// The total number of bytes processed by this instance
            stats::Stat<size_t> qubitsReceived {{parent, "Qubits Received"}, stats::Units::Count};

            /// The time took to transmit the qubits
            stats::Stat<double> timeTaken {{parent, "Time Taken"}, stats::Units::Milliseconds};

            /// The time took to transmit the qubits
            stats::Stat<double> frameTime {{parent, "Frame Time"}, stats::Units::Milliseconds};

            /// @copydoc stats::StatCollection::Add
            virtual void Add(stats::IAllStatsCallback* statsCb) override
            {
                qubitsReceived.Add(statsCb);
                qubitsTransmitted.Add(statsCb);
                timeTaken.Add(statsCb);
                frameTime.Add(statsCb);
            }

            /// @copydoc stats::StatCollection::Remove
            virtual void Remove(stats::IAllStatsCallback* statsCb) override
            {
                qubitsReceived.Remove(statsCb);
                qubitsTransmitted.Remove(statsCb);
                timeTaken.Remove(statsCb);
                frameTime.Remove(statsCb);
            }

            /// @copydoc stats::StatCollection::AllStats
            std::vector<stats::StatBase*> AllStats() override
            {
                return
                {
                    &qubitsTransmitted, &qubitsReceived, &timeTaken, &frameTime
                };
            }
        }; // struct Statistics

    } // namespace sim
} // namespace cqp
