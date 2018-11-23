/*!
        virtual bool WaitForRawData(const SessionID& session, const FrameID& frame) = 0;
* @file
* @brief CQP Toolkit - Detection Reports
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Datatypes/DetectionReport.h"
#include "CQPToolkit/Util/Platform.h"

namespace cqp
{

    /// @brief Receive notification of interpreted photon detections.
    /// @details Called by implementers of IDetectionReports when registered using IEvent
    class CQPTOOLKIT_EXPORT IDetectionEventCallback
    {
    public:

        /// @brief Notification of new photon detection
        /// @param[in] report The details of the detections
        virtual void OnPhotonReport(std::unique_ptr<ProtocolDetectionReport> report)=0;

        /// Pure virtual destructor
        virtual ~IDetectionEventCallback() = default;
    };

}
