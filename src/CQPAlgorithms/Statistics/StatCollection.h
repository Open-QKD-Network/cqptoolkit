/*!
* @file
* @brief StatCollection
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 5/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <vector>
#include <string>
#include "CQPAlgorithms/Statistics/Stat.h"

namespace cqp
{

    namespace stats
    {
        class IAllStatsCallback;

        /**
         * @brief The StatCollection struct
         * Holds statistics for a given functional area
         */
        struct CQPALGORITHMS_EXPORT StatCollection
        {
        public:
            /**
             * @brief Add
             * register the statistics listener with all the stats in this collections
             * @param statsCb Listener to register with
             */
            virtual void Add(IAllStatsCallback* statsCb) = 0;
            /**
             * @brief Remove
             * Unregister the listener
             * @param statsCb Listener to unregister
             */
            virtual void Remove(IAllStatsCallback* statsCb) = 0;

            /**
             * @brief SetEndpoints
             * Set the extra parameters which state which end points the stats belong to
             * @param from
             * @param to
             */
            void SetEndpoints(const std::string& from, const std::string& to)
            {
                for(auto stat : AllStats())
                {
                    stat->parameters["from"] = from;
                    stat->parameters["to"] = to;
                }
            }

            /// Destructor
            virtual ~StatCollection() {}
        protected:

            /**
             * @brief AllStats
             * @return A list of all stats in the collection
             */
            virtual std::vector<StatBase*> AllStats() = 0;
        }; // struct StatCollection
    } // namespace stats
} // namespace cqp
