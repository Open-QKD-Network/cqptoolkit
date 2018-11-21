/*!
* @file
* @brief StatisticsLogger
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "StatisticsLogger.h"
#include "CQPToolkit/Util/ConsoleLogger.h"

namespace cqp
{
    namespace stats
    {

        StatisticsLogger::StatisticsLogger()
        {
            ConsoleLogger::Enable();
        }

        std::string StatisticsLogger::StatTree(const stats::StatBase* whichStat)
        {
            using namespace std;
            string result;
            auto currentName = whichStat->GetPath().begin();
            if(currentName != whichStat->GetPath().end())
            {
                result = *currentName;
                currentName++;

                while(currentName != whichStat->GetPath().end())
                {
                    result += ":" + *currentName;
                    currentName++;
                }
            }

            return result;
        }

        std::string StatisticsLogger::StatUnit(stats::Units unit)
        {
            using namespace std;
            string result = "units: \"";

            switch (unit)
            {
            case stats::Units::Complex:
            // no symbol
            case stats::Units::Count:
                // no symbol
                break;
            case stats::Units::Milliseconds:
                result += "ms";
                break;
            case stats::Units::Percentage:
                result += "%";
                break;
            case stats::Units::Decibels:
                result += "dB";
                break;
            case stats::Units::Hz:
                result += "Hz";
                break;

            }
            result += "\"";

            return result;

        }
    } // namespace stats
} // namespace cqp

