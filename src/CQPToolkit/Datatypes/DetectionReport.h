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

#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Datatypes/Qubits.h"
#include <chrono>
#include <queue>
#include <memory>

namespace cqp
{

    /// A unique identifier for a detector within the system
    /// @remarks This should probably be a UUID.
    using DetectorId = unsigned int;

    /// A structure for storing arrival times of photons
    enum class TimeFormat
    {
        RawClock_Taps,
        Absolute,
        ClockOffset
    };

    /// A definition of time for use with time tagging
    using PicoSeconds = std::chrono::duration<uint64_t, std::pico>;

    /// The data produced by a Time tagger/Time digitiser once a detector has
    /// been triggered.
    struct CQPTOOLKIT_EXPORT DetectionReport
    {
    public:
        /// The moment at which the event was detected.
        /// @note
        /// This is different to the value sent by some hardware
        /// Often this will be converted from a course free running clock + tick offset.
        /// investigations need to be performed to decide whether it is worth processing between the two forms.
        PicoSeconds time {0};
        /// Some identifier for the detector
        Qubit value {0};

    };

    /**
     * @brief operator ==
     * Compare detection reports
     * @param left
     * @param right
     * @return true if the time and value are additional     */
    inline bool operator==(const DetectionReport& left, const DetectionReport& right)
    {
        return left.value == right.value && left.time == right.time;
    }

    /// A list of detection reports
    using DetectionReportList = std::vector<DetectionReport>;

    /// Stores the data report with the additional information about which frame it arrived in.
    struct CQPTOOLKIT_EXPORT ProtocolDetectionReport
    {
        /// The frame which this detection belongs
        SequenceNumber frame;
        /// The detections time stamp os relative to this point in time.
        std::chrono::high_resolution_clock::time_point epoc;
        /// The detection report
        DetectionReportList detections;
    };

    /// Stores the data report with the additional information about which frame it arrived in.
    struct CQPTOOLKIT_EXPORT EmitterReport
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
