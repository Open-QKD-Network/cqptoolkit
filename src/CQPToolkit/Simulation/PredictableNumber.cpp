#include "PredictableNumber.h"
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

#include "PredictableNumber.h"
#include <random>


namespace cqp
{

    /**
     * This function will return a specific number of random bit from a pre-generated
     * pool of numbers.
     */
    ulong PredictableNumber::RandULong()
    {
        ulong result = data[currentPos];
        SetPosition(currentPos + 1);
        return result;
    }

    Qubit PredictableNumber::RandQubit()
    {
        Qubit result;
        result = static_cast<Qubit>(RandULong() % 3);
        return result;
    }

    void PredictableNumber::SetPosition(unsigned int index)
    {
        if (index >= data.size())
        {
            currentPos = 0;
        }
        else
        {
            currentPos = index;
        }
    }
}
