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
#if !defined(EA_CCF4FE42_0BF9_4f8f_AEEE_904062EFBC2C__INCLUDED_)
#define EA_CCF4FE42_0BF9_4f8f_AEEE_904062EFBC2C__INCLUDED_
#include <queue>
#include "CQPToolkit/Interfaces/IRandom.h"
#include <future>
#include "CQPToolkit/Util/WorkerThread.h"
#include <random>

#if defined(_MSC_VER_)
    #pragma warning(push)
    #pragma warning(disable:4251)
#endif

namespace cqp
{
    /// Simple source of random numbers for simulation.
    /// @todo add functions for getting different amounts of random numbers as needed
    /// @todo Provide a means of controling the method/ditrobution of numbers for simulation.
    class CQPTOOLKIT_EXPORT RandomNumber : public virtual IRandom
    {

    public:
        /// Default constructor
        RandomNumber();
        /// Default destructor
        virtual ~RandomNumber() override = default;
        /// @copydoc IRandom::RandULong
        ulong RandULong() override;

        /// return a single random number
        /// @returns a random number
        static int SRandInt();

        /// @copydoc IRandom::RandQubit
        Qubit RandQubit() override;

        /**
         * @brief RandQubitList
         * @param NumQubits The number of qubits to return
         * @return A random selection of valid Qubits
         */
        QubitList RandQubitList(size_t NumQubits);
    protected:
        /// Distribution algorithms to ensure good distribution of numbers
        std::uniform_int_distribution<ulong> intDistribution;
        /// Distribution algorithms to ensure good distribution of numbers
        std::uniform_int_distribution<int> qubitDistribution;
        /// Random number generator
        std::default_random_engine generator;
    };
}
#endif // !defined(EA_CCF4FE42_0BF9_4f8f_AEEE_904062EFBC2C__INCLUDED_)

#if defined(_MSC_VER_)
    #pragma warning(pop)
#endif
