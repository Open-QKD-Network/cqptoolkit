/*!
* @file
* @brief CQP Toolkit - Random Number Generator
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <queue>
#include "Algorithms/Random/IRandom.h"
#include <future>
#include "Algorithms/Util/WorkerThread.h"
#include <random>

#if defined(_MSC_VER_)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif

namespace cqp
{
    /// Simple source of random numbers for simulation.
    /// @todo add functions for getting different amounts of random numbers as needed
    /// @todo Provide a means of controlling the method/distribution of numbers for simulation.
    class ALGORITHMS_EXPORT RandomNumber : public virtual IRandom
    {

    public:
        /// Default constructor
        RandomNumber();
        /// Default destructor
        virtual ~RandomNumber() override = default;
        /// @copydoc IRandom::RandULong
        ulong RandULong() override;

        /// @copydoc IRandom::RandomBytes
        void RandomBytes(size_t numOfBytes, DataBlock& dest) override;

        /// return a single random number
        /// @returns a random number
        static int SRandInt();

        /// return a single random qubit
        /// @returns a random qubit
        Qubit RandQubit();

        /// @copydoc IRandom::RandQubitList
        QubitList RandQubitList(size_t numQubits) override;
    protected:
        /// Distribution algorithms to ensure good distribution of numbers
        std::uniform_int_distribution<ulong> intDistribution;
        /// Distribution algorithms to ensure good distribution of numbers
        std::uniform_int_distribution<uint8_t> qubitDistribution;
        /// Random number generator
        std::default_random_engine generator;

    };
}

#if defined(_MSC_VER_)
    #pragma warning(pop)
#endif
