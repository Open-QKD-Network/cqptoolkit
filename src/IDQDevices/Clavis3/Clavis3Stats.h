/*!
* @file
* @brief Clavis3Stats
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 24/8/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Statistics/StatCollection.h"

namespace cqp
{

    struct Clavis3Stats : public stats::StatCollection
    {
        Clavis3Stats() {}
        /// The group to contain these stats
        const char* parent = "Clavis3";

        stats::Stat<size_t> SystemState_Changed {{parent, "SystemState"}, stats::Units::Count, "SystemState Changed"};
        stats::Stat<size_t> PowerupComponentsState_Changed {{parent, "PowerupComponentsState"}, stats::Units::Count, "PowerupComponentsState Changed"};
        stats::Stat<size_t> AlignmentState_Changed {{parent, "AlignmentState"}, stats::Units::Count, "AlignmentState Changed"};
        stats::Stat<size_t> OptimizingOpticsState_Changed {{parent, "OptimizingOpticsState"}, stats::Units::Count, "OptimizingOpticsState Changed"};
        stats::Stat<size_t> ShutdownState_Changed {{parent, "ShutdownState"}, stats::Units::Count, "ShutdownState Changed"};
        stats::Stat<size_t> TimebinAlignmentPattern_Changed {{parent, "TimebinAlignmentPattern"}, stats::Units::Count, "TimebinAlignmentPattern Changed"};
        stats::Stat<size_t> FrameAlignmentPattern_Changed {{parent, "FrameAlignmentPattern"}, stats::Units::Count, "FrameAlignmentPattern Changed"};


        stats::Stat<double> Laser_BiasCurrent {{parent, "Laser", "BiasCurrent"}, stats::Units::Count, "Laser BiasCurrent"};
        stats::Stat<double> Laser_Temperature {{parent, "Laser", "Temperature"}, stats::Units::Count, "Laser Temperature"};
        stats::Stat<double> Laser_Power {{parent, "Laser", "Power"}, stats::Units::Count, "Laser Power"};
        stats::Stat<double> Laser_TECCurrent {{parent, "Laser", "TECCurrent"}, stats::Units::Count, "Laser TECCurrent"};
        stats::Stat<double> IM_BiasVoltage {{parent, "IM", "BiasVoltage"}, stats::Units::Count, "IM BiasVoltage"};
        stats::Stat<double> IM_AmplifierCurrent {{parent, "IM", "AmplifierCurrent"}, stats::Units::Count, "IM AmplifierCurrent"};
        stats::Stat<double> IM_AmplifierVoltage {{parent, "IM", "AmplifierVoltage"}, stats::Units::Count, "IM AmplifierVoltage"};
        stats::Stat<double> IM_Temperature {{parent, "IM", "Temperature"}, stats::Units::Count, "IM Temperature"};
        stats::Stat<double> IM_TECCurrent {{parent, "IM", "TECCurrent"}, stats::Units::Count, "IM TECCurrent"};
        stats::Stat<double> VOA_Attenuation {{parent, "VOA", "Attenuation"}, stats::Units::Count, "VOA Attenuation"};
        stats::Stat<double> MonitoringPhotodiode_Power {{parent, "MonitoringPhotodiode", "Power"}, stats::Units::Count, "MonitoringPhotodiode Power"};
        stats::Stat<double> IF_Temperature {{parent, "IF", "Temperature"}, stats::Units::Count, "IF Temperature"};
        stats::Stat<double> Filter_Temperature {{parent, "Filter", "Temperature"}, stats::Units::Count, "Filter Temperature"};
        stats::Stat<double> DataDetector_Temperature {{parent, "DataDetector", "Temperature"}, stats::Units::Count, "DataDetector Temperature"};
        stats::Stat<double> DataDetector_BiasCurrent {{parent, "DataDetector", "BiasCurrent"}, stats::Units::Count, "DataDetector BiasCurrent"};
        stats::Stat<double> MonitorDetector_Temperature {{parent, "MonitorDetector", "Temperature"}, stats::Units::Count, "MonitorDetector Temperature"};
        stats::Stat<double> MonitorDetector_BiasCurrent {{parent, "MonitorDetector", "BiasCurrent"}, stats::Units::Count, "MonitorDetector BiasCurrent"};
        stats::Stat<double> Qber {{parent, "Qber"}, stats::Units::Percentage, "Quantum Bit Error Rate"};
        stats::Stat<double> Visibility {{parent, "Visibility"}, stats::Units::Percentage, "Visibility"};

        /// @copydoc stats::StatCollection::Add
        void Add(stats::IAllStatsCallback* statsCb) override
        {
            SystemState_Changed.Add(statsCb);
            PowerupComponentsState_Changed.Add(statsCb);
            AlignmentState_Changed.Add(statsCb);
            OptimizingOpticsState_Changed.Add(statsCb);
            ShutdownState_Changed.Add(statsCb);
            TimebinAlignmentPattern_Changed.Add(statsCb);
            FrameAlignmentPattern_Changed.Add(statsCb);

            Laser_BiasCurrent.Add(statsCb);
            Laser_Temperature.Add(statsCb);
            Laser_Power.Add(statsCb);
            Laser_TECCurrent.Add(statsCb);
            IM_BiasVoltage.Add(statsCb);
            IM_AmplifierCurrent.Add(statsCb);
            IM_AmplifierVoltage.Add(statsCb);
            IM_Temperature.Add(statsCb);
            IM_TECCurrent.Add(statsCb);
            VOA_Attenuation.Add(statsCb);
            MonitoringPhotodiode_Power.Add(statsCb);
            IF_Temperature.Add(statsCb);
            Filter_Temperature.Add(statsCb);
            DataDetector_Temperature.Add(statsCb);
            DataDetector_BiasCurrent.Add(statsCb);
            MonitorDetector_Temperature.Add(statsCb);
            MonitorDetector_BiasCurrent.Add(statsCb);
            Qber.Add(statsCb);
            Visibility.Add(statsCb);
        }

        /// @copydoc stats::StatCollection::Remove
        void Remove(stats::IAllStatsCallback* statsCb) override
        {
            SystemState_Changed.Remove(statsCb);
            PowerupComponentsState_Changed.Remove(statsCb);
            AlignmentState_Changed.Remove(statsCb);
            OptimizingOpticsState_Changed.Remove(statsCb);
            ShutdownState_Changed.Remove(statsCb);
            TimebinAlignmentPattern_Changed.Remove(statsCb);
            FrameAlignmentPattern_Changed.Remove(statsCb);

            Laser_BiasCurrent.Remove(statsCb);
            Laser_Temperature.Remove(statsCb);
            Laser_Power.Remove(statsCb);
            Laser_TECCurrent.Remove(statsCb);
            IM_BiasVoltage.Remove(statsCb);
            IM_AmplifierCurrent.Remove(statsCb);
            IM_AmplifierVoltage.Remove(statsCb);
            IM_Temperature.Remove(statsCb);
            IM_TECCurrent.Remove(statsCb);
            VOA_Attenuation.Remove(statsCb);
            MonitoringPhotodiode_Power.Remove(statsCb);
            IF_Temperature.Remove(statsCb);
            Filter_Temperature.Remove(statsCb);
            DataDetector_Temperature.Remove(statsCb);
            DataDetector_BiasCurrent.Remove(statsCb);
            MonitorDetector_Temperature.Remove(statsCb);
            MonitorDetector_BiasCurrent.Remove(statsCb);
            Qber.Remove(statsCb);
            Visibility.Remove(statsCb);
        }
    };

} // namespace cqp


