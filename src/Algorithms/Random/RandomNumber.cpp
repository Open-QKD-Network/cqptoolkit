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

#include "RandomNumber.h"
#include <chrono>
#include <thread>


namespace cqp
{

    RandomNumber::RandomNumber() :
        qubitDistribution(0, static_cast<int>(BB84::Neg)),
        generator(static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count()))
    {
    }

    int RandomNumber::SRandInt()
    {
        return rand();
    }

    /**
     * This function will return a specific number of random bit from a pre-generated
     * pool of numbers.
     */
    uint64_t RandomNumber::RandULong()
    {

        return intDistribution(generator);
    }

    Qubit RandomNumber::RandQubit()
    {
        Qubit result;
        // this will only generate upto four states
        result = static_cast<Qubit>(qubitDistribution(generator));
        return result;
    }

    QubitList RandomNumber::RandQubitList(size_t numQubits)
    {
        QubitList outputQubits;
        outputQubits.reserve(numQubits);
        for(size_t i = 0; i < numQubits; i++)
        {
            outputQubits.push_back(RandQubit());
        }
        return outputQubits;
    }

    void RandomNumber::RandomBytes(size_t numOfBytes, DataBlock& dest)
    {
        dest.reserve(numOfBytes);
        for(size_t i = 0; i < numOfBytes; i++)
        {
            dest.push_back(static_cast<DataBlock::value_type>(qubitDistribution(generator)));
        }
    }
}

