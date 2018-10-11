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
#include "CQPToolkit/Interfaces/IRandom.h"
#include <future>
#include "CQPToolkit/Util/RingBuffer.hpp"
#include "CQPToolkit/Util/WorkerThread.h"

namespace cqp
{
    /// Simple source of random numbers for simulation.
    /// @todo add functions for getting different amounts of random numbers as needed
    /// @todo Provide a means of controling the method/ditrobution of numbers for simulation.
    class CQPTOOLKIT_EXPORT PredictableNumber : public virtual IRandom
    {

    public:
        /// Default constructor
        PredictableNumber();
        /// Default distructor
        virtual ~PredictableNumber() override;
        /// return a single random number
        /// @return a random integer
        ulong RandULong() override;
        /// Generate a random qubit
        /// @return A random qubit
        Qubit RandQubit() override;
        /// Change the point in the table from which the next random number will be taken
        /// @param index table position
        void SetPosition(unsigned int index);
    protected:
        /// The current index into the data
        unsigned int currentPos = 0;
        /// precalcualted values
        const std::vector<long> data =
        {
            -15702,23351,63325,
                39605,-35444,49264,
                -42580,-26551,50689,
                -52806,-3723,-6931,
                -44309,-19604,64359,
                -24170,-16346,18500,
                14870,-64961,-39446,
                59084,-37856,4144,
                -45614,-8383,42819,
                28287,48717,64022,
                -48968,-32629,-52118,
                -7685,-43038,-54041,
                -33389,-9352,-46842,
                31144,24571,49983,
                24596,-34351,-40097,
                43175,-34200,-39563,
                22385,-50942,43017,
                37698,-51675,-40925,
                40438,46055,54017,
                -45952,25025,-32078,
                -65089,26134,30215,
                -56924,-22125,-11195,
                63859,28908,30540,
                -4256,-43664,16925,
                47468,-40454,65325,
                -37917,38784,16296,
                14448,36401,11665,
                -62975,-51978,-60358,
                -40432,-11708,54639,
                9474,-664,59484,
                -45344,-10928,-7808,
                58210,51068,48693,
                30289,14365,-24663,
                -48420,-6388,55019,
                -16088,36603,-7566,
                -65403,-22111,4582,
                37157,16942,19499,
                -30554,-15936,-17215,
                -62875,4166,48176,
                -59911,47714,-21121,
                54648,-13517,-55389,
                21175,-49236,-25991,
                54278,-8140,-64122,
                -57444,43971,-869,
                9638,62638,-44507,
                -56144,-3928,-51633,
                60279,30122,52026,
                -1562,40232,12009,
                -36633,-46505,-46769,
                44455,-62054,60190,
                42918,35686,-43170,
                -35640,-54761,8367,
                -17156,-30312,-55830,
                31593,33848,-29170,
                1757,382,56759,
                37235,663,20858,
                -50796,10064,-14604,
                30620,32697,33250,
                62822,38930,-33173,
                -1109,-42375,12693,
                -758,-38020,-47197,
                -22717,-38646,42966,
                6921,-50971,2351,
                24650,-29753,-5012,
                -25,45697,-23109,
                17477,49525,-38445,
                -64671,43323,-2178,
                -52583,24973,14828,
                21882,-14837,62992,
                59438,58489,-8567,
                -39491,7775,43334,
                48111,5065,-23621,
                47167,56940,16762,
                6910,-2784,-23605,
                -56296,-34953,-12576,
                44052,41913,13313,
                -10218,31719,19623,
                24843,31972,706,
                -9277,59597,61946,
                10779,26916,-44164,
                4094,-36057,55387,
                -8237,-48561,42919,
                52948,-31596,-6347,
                64104,49934,-13327,
                19408,-41359,17577,
                18275,-65390,-26558,
                -12397,-11123,63048,
                -1240,37168,22770,
                -7794,5653,4157,
                14640,-39733,-57882,
                -27577,15786,31958,
                -26569,-65481,-593,
                62292,-29117,-55630,
                -8744,-446,59501,
                -23207,-19667,14412,
                -64387,-60271,-48092,
                16344,-4966,26552,
                -39514,-42817,8283,
                -55172,-15465,54487,
                22976,-53243,-1957
            };
    };
}
