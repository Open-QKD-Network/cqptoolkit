/*!
* @file
* @brief CQP Toolkit - DetectionReport
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "Algorithms/algorithms_export.h"
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/Chrono.h"
#include <chrono>
#include <vector>
#include <queue>
#include <memory>

namespace cqp
{

    /// A unique identifier for a detector within the system
    /// @remarks This should probably be a UUID.
    using DetectorId = unsigned int;

    using DetectionTimes = std::vector<PicoSeconds>;

    /// The data produced by a Time tagger/Time digitiser once a detector has
    /// been triggered.
    struct ALGORITHMS_EXPORT DetectionReports
    {
    public:
        /// The moment at which the event was detected.
        /// @note
        /// This is different to the value sent by some hardware
        /// Often this will be converted from a course free running clock + tick offset.
        /// investigations need to be performed to decide whether it is worth processing between the two forms.
        DetectionTimes times;
        /// Some identifier for the detector
        QubitList values;

        void Reserve(size_t size)
        {
            times.reserve(size);
            values.reserve(size);
        }

        size_t Size()
        {
            return times.size();
        }

        void Clear()
        {
            times.clear();
            values.clear();
        }
    };

    /// Stores the data report with the additional information about which frame it arrived in.
    struct ALGORITHMS_EXPORT ProtocolDetectionReport
    {
        /// The frame which this detection belongs
        SequenceNumber frame;
        /// The detections time stamp os relative to this point in time.
        std::chrono::high_resolution_clock::time_point epoc;
        /// The detection report
        DetectionReports detections;
    };

    /// Stores the data report with the additional information about which frame it arrived in.
    struct ALGORITHMS_EXPORT EmitterReport
    {
        /// The frame which this detection belongs
        SequenceNumber frame;
        /// The detections time stamp os relative to this point in time.
        std::chrono::high_resolution_clock::time_point epoc;
        /// the time between photon emissions
        PicoSeconds period;
        /// The detection report
        QubitList emissions;
    };

    /// A list of ProtocolDetectionReports
    using ProtocolDetectionReportList = std::queue<std::unique_ptr<ProtocolDetectionReport>>;
    /// A list of ProtocolDetectionReports
    using EmitterReportList = std::unordered_map<SequenceNumber /*frame*/, std::unique_ptr<EmitterReport>>;
}
