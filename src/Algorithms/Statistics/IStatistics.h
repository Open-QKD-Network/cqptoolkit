/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/IEvent.h"
#include "Algorithms/algorithms_export.h"
#include <cstddef>

namespace cqp
{
    namespace stats
    {

        /// Forward declaration of Stat
        template<typename T>
        class ALGORITHMS_EXPORT Stat;

        /**
         * Callback interface for receiving stats
         * @tparam T Datatype which the stat will store
         */
        template<typename T>
        class ALGORITHMS_EXPORT IStatCallback
        {
        public:
            /**
             * @brief StatUpdated
             * Notification that a stat has been updated
             * @param stat The stat which has been updated
             */
            virtual void StatUpdated(const Stat<T>* stat) = 0;
            /// Destructor
            virtual ~IStatCallback() = default;
        };

        /**
         * @brief The IAllStatsCallback class
         * Callback interface for receiving all datatypes
         */
        class ALGORITHMS_EXPORT IAllStatsCallback :
            public virtual IStatCallback<double>,
            public virtual IStatCallback<long>,
            public virtual IStatCallback<size_t>
        {

        };

    } // namespace stats
} // namespace cqp
