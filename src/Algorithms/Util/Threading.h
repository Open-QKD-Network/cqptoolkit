/*!
* @file
* @brief CQP Toolkit - Worker thread helper
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28 Jan 2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <thread>
#include "Algorithms/algorithms_export.h"

namespace cqp
{

    /// platform independent definitions of scheduling methods
    /// these do not directly translate on every platform
    enum class Scheduler { Idle, Batch, Normal, RoundRobin, FIFO, Deadline};

    /**
     * @brief SetPriority
     * Change a threads priority
     * @param theThread
     * @param niceLevel Higher number == less chance it will run, more nice
     * @param realtimePriority Higher number == more chance it will run
     * @param policy The kind of scheduler to use
     * @return true on success
     */
    ALGORITHMS_EXPORT bool SetPriority(std::thread& theThread, int niceLevel, Scheduler policy = Scheduler::Normal, int realtimePriority = 1);

}
