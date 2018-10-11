/*!
* @file
* @brief AlignmentTestData
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Datatypes/DetectionReport.h"

namespace cqp
{
    namespace test
    {

        class AlignmentTestData
        {
        public:
            QubitList emissions;
            PicoSeconds emissionPeriod {100000};
            PicoSeconds emissionDelay  {1000};
            DetectionReportList detections;

            void LoadGated(const std::string& txFile);
            void LoadBobDetections(const std::string& txFile);
        };

    } // namespace test
} // namespace cqp


