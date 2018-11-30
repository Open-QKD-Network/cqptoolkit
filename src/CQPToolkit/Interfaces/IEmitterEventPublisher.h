/*!
* @brief CQP Toolkit - Detection Reports
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 28 Sep 2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPAlgorithms/Datatypes/DetectionReport.h"


namespace cqp
{

    /// @brief Receive notification of interpreted photon detections.
    /// @details Called by implementers of IDetectionReports when registered using IEvent
    class CQPTOOLKIT_EXPORT IEmitterEventCallback
    {
    public:

        /// @brief Notification of new photon detection
        /// @param[in] report The details of the detections
        virtual void OnEmitterReport(std::unique_ptr<EmitterReport> report)=0;

        /// Pure virtual destructor
        virtual ~IEmitterEventCallback() = default;
    };

}
