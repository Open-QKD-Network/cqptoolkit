/*!
* @file
* @brief Clavis3SignalHandler
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/7/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(HAVE_IDQ4P)

//Software Signals
#include "Signals/OnSystemState_Changed.hpp"
#include "Signals/OnUpdateSoftware_Progress.hpp"
#include "Signals/OnPowerUpComponentsState_Changed.hpp"
#include "Signals/OnAlignmentState_Changed.hpp"
#include "Signals/OnOptimizingOpticsState_Changed.hpp"
#include "Signals/OnShutdownState_Changed.hpp"
#include "Signals/OnKeySecurity_OutOfRange.hpp"
#include "Signals/OnKeyAuthentication_Mismatch.hpp"
#include "Signals/OnKeySecurity_SingleFailure.hpp"
#include "Signals/OnKeySecurity_RepeatedFailure.hpp"
#include "Signals/OnKeyDeliverer_Exception.hpp"
#include "Signals/OnCommandServer_Exception.hpp"
//Alice
#include "Signals/Alice/OnLaser_BiasCurrent_NewValue.hpp"
#include "Signals/Alice/OnLaser_BiasCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnLaser_BiasCurrent_OperationOutOfRange.hpp"
#include "Signals/Alice/OnLaser_Temperature_NewValue.hpp"
#include "Signals/Alice/OnLaser_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnLaser_Temperature_OperationOutOfRange.hpp"
#include "Signals/Alice/OnLaser_Power_NewValue.hpp"
#include "Signals/Alice/OnLaser_Power_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnLaser_Power_OperationOutOfRange.hpp"
#include "Signals/Alice/OnLaser_TECCurrent_NewValue.hpp"
#include "Signals/Alice/OnLaser_TECCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnLaser_TECCurrent_OperationOutOfRange.hpp"
#include "Signals/Alice/OnIM_BiasVoltage_NewValue.hpp"
#include "Signals/Alice/OnIM_BiasVoltage_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnIM_BiasVoltage_OperationOutOfRange.hpp"
#include "Signals/Alice/OnIM_AmplifierCurrent_NewValue.hpp"
#include "Signals/Alice/OnIM_AmplifierCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnIM_AmplifierCurrent_OperationOutOfRange.hpp"
#include "Signals/Alice/OnIM_AmplifierVoltage_NewValue.hpp"
#include "Signals/Alice/OnIM_AmplifierVoltage_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnIM_AmplifierVoltage_OperationOutOfRange.hpp"
#include "Signals/Alice/OnIM_Temperature_NewValue.hpp"
#include "Signals/Alice/OnIM_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnIM_Temperature_OperationOutOfRange.hpp"
#include "Signals/Alice/OnIM_TECCurrent_NewValue.hpp"
#include "Signals/Alice/OnIM_TECCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnIM_TECCurrent_OperationOutOfRange.hpp"
#include "Signals/Alice/OnVOA_Attenuation_NewValue.hpp"
#include "Signals/Alice/OnVOA_Attenuation_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnMonitoringPhotodiode_Power_NewValue.hpp"
#include "Signals/Alice/OnMonitoringPhotodiode_Power_AbsoluteOutOfRange.hpp"
#include "Signals/Alice/OnMonitoringPhotodiode_Power_OperationOutOfRange.hpp"
//Bob
#include "Signals/Bob/OnIF_Temperature_NewValue.hpp"
#include "Signals/Bob/OnIF_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnIF_Temperature_OperationOutOfRange.hpp"
#include "Signals/Bob/OnFilter_Temperature_NewValue.hpp"
#include "Signals/Bob/OnFilter_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnFilter_Temperature_OperationOutOfRange.hpp"
#include "Signals/Bob/OnDataDetector_Temperature_NewValue.hpp"
#include "Signals/Bob/OnDataDetector_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnDataDetector_Temperature_OperationOutOfRange.hpp"
#include "Signals/Bob/OnDataDetector_BiasCurrent_NewValue.hpp"
#include "Signals/Bob/OnDataDetector_BiasCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnDataDetector_BiasCurrent_OperationOutOfRange.hpp"
#include "Signals/Bob/OnMonitorDetector_Temperature_NewValue.hpp"
#include "Signals/Bob/OnMonitorDetector_Temperature_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnMonitorDetector_Temperature_OperationOutOfRange.hpp"
#include "Signals/Bob/OnMonitorDetector_BiasCurrent_NewValue.hpp"
#include "Signals/Bob/OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange.hpp"
#include "Signals/Bob/OnMonitorDetector_BiasCurrent_OperationOutOfRange.hpp"
//FPGA
#include "Signals/FPGA/OnQber_NewValue.hpp"
#include "Signals/FPGA/OnVisibility_NewValue.hpp"
#include "Signals/FPGA/OnFPGA_Failure.hpp"
//Supplemental signals for manual mode
//#include "Signals/OnTimebinAlignmentPattern_Changed.hpp"
//#include "Signals/OnFrameAlignmentPattern_Changed.hpp"
//#include "Signals/OnOpticsOptimization_InProgress.hpp"

#include "MsgpackSerializer.hpp"
namespace cqp
{

    class Clavis3SignalHandler
    {
    public:
        Clavis3SignalHandler();
        //Software Signals
        static idq4p::classes::OnSystemState_Changed Decode_OnSystemState_Changed(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnUpdateSoftware_Progress Decode_OnUpdateSoftware_Progress(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnPowerUpComponentsState_Changed Decode_OnPowerupComponentsState_Changed(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnAlignmentState_Changed Decode_OnAlignmentState_Changed(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnOptimizingOpticsState_Changed Decode_OnOptimizingOpticsState_Changed(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnShutdownState_Changed Decode_OnShutdownState_Changed(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnKeySecurity_OutOfRange Decode_OnKeySecurity_OutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnKeyAuthentication_Mismatch Decode_OnKeyAuthentication_Mismatch(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnKeySecurity_SingleFailure Decode_OnKeySecurity_SingleFailure(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnKeySecurity_RepeatedFailure Decode_OnKeySecurity_RepeatedFailure(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnKeyDeliverer_Exception Decode_OnKeyDeliverer_Exception(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnCommandServer_Exception Decode_OnCommandServer_Exception(const msgpack::sbuffer& buffer);
        //Alice
        static idq4p::classes::OnLaser_BiasCurrent_NewValue Decode_OnLaser_BiasCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_BiasCurrent_AbsoluteOutOfRange Decode_OnLaser_BiasCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_BiasCurrent_OperationOutOfRange Decode_OnLaser_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Temperature_NewValue Decode_OnLaser_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Temperature_AbsoluteOutOfRange Decode_OnLaser_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Temperature_OperationOutOfRange Decode_OnLaser_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Power_NewValue Decode_OnLaser_Power_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Power_AbsoluteOutOfRange Decode_OnLaser_Power_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_Power_OperationOutOfRange Decode_OnLaser_Power_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_TECCurrent_NewValue Decode_OnLaser_TECCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_TECCurrent_AbsoluteOutOfRange Decode_OnLaser_TECCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnLaser_TECCurrent_OperationOutOfRange Decode_OnLaser_TECCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_BiasVoltage_NewValue Decode_OnIM_BiasVoltage_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_BiasVoltage_AbsoluteOutOfRange Decode_OnIM_BiasVoltage_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_BiasVoltage_OperationOutOfRange Decode_OnIM_BiasVoltage_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierCurrent_NewValue Decode_OnIM_AmplifierCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierCurrent_AbsoluteOutOfRange Decode_OnIM_AmplifierCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierCurrent_OperationOutOfRange Decode_OnIM_AmplifierCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierVoltage_NewValue Decode_OnIM_AmplifierVoltage_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierVoltage_AbsoluteOutOfRange Decode_OnIM_AmplifierVoltage_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_AmplifierVoltage_OperationOutOfRange Decode_OnIM_AmplifierVoltage_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_Temperature_NewValue Decode_OnIM_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_Temperature_AbsoluteOutOfRange Decode_OnIM_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_Temperature_OperationOutOfRange Decode_OnIM_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_TECCurrent_NewValue Decode_OnIM_TECCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_TECCurrent_AbsoluteOutOfRange Decode_OnIM_TECCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIM_TECCurrent_OperationOutOfRange Decode_OnIM_TECCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnVOA_Attenuation_NewValue Decode_OnVOA_Attenuation_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnVOA_Attenuation_AbsoluteOutOfRange Decode_OnVOA_Attenuation_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitoringPhotodiode_Power_NewValue Decode_OnMonitoringPhotodiode_Power_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitoringPhotodiode_Power_AbsoluteOutOfRange  Decode_OnMonitoringPhotodiode_Power_AbsoluteOutOfRange (const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitoringPhotodiode_Power_OperationOutOfRange Decode_OnMonitoringPhotodiode_Power_OperationOutOfRange(const msgpack::sbuffer& buffer);
        //Bob
        static idq4p::classes::OnIF_Temperature_NewValue Decode_OnIF_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIF_Temperature_AbsoluteOutOfRange Decode_OnIF_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnIF_Temperature_OperationOutOfRange Decode_OnIF_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnFilter_Temperature_NewValue Decode_OnFilter_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnFilter_Temperature_AbsoluteOutOfRange Decode_OnFilter_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnFilter_Temperature_OperationOutOfRange Decode_OnFilter_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_Temperature_NewValue Decode_OnDataDetector_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_Temperature_AbsoluteOutOfRange Decode_OnDataDetector_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_Temperature_OperationOutOfRange Decode_OnDataDetector_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_BiasCurrent_NewValue Decode_OnDataDetector_BiasCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_BiasCurrent_AbsoluteOutOfRange Decode_OnDataDetector_BiasCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnDataDetector_BiasCurrent_OperationOutOfRange Decode_OnDataDetector_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_Temperature_NewValue Decode_OnMonitorDetector_Temperature_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_Temperature_AbsoluteOutOfRange  Decode_OnMonitorDetector_Temperature_AbsoluteOutOfRange (const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_Temperature_OperationOutOfRange Decode_OnMonitorDetector_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_BiasCurrent_NewValue Decode_OnMonitorDetector_BiasCurrent_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange  Decode_OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange (const msgpack::sbuffer& buffer);
        static idq4p::classes::OnMonitorDetector_BiasCurrent_OperationOutOfRange Decode_OnMonitorDetector_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer);
        //FPGA
        static idq4p::classes::OnQber_NewValue Decode_OnQber_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnVisibility_NewValue Decode_OnVisibility_NewValue(const msgpack::sbuffer& buffer);
        static idq4p::classes::OnFPGA_Failure Decode_OnFPGA_Failure(const msgpack::sbuffer& buffer);
        //Supplemental signals for manual mode
        /*static idq4p::classes::OnTimebinAlignmentPattern_Changed Decode_OnTimebinAlignmentPattern_Changed(const msgpack::sbuffer& buffer) { OnTimebinAlignmentPattern_Changed signal; MsgpackSerializer::Deserialize<OnTimebinAlignmentPattern_Changed>(buffer, signal); return signal; }
        static idq4p::classes::OnFrameAlignmentPattern_Changed Decode_OnFrameAlignmentPattern_Changed(const msgpack::sbuffer& buffer) { OnFrameAlignmentPattern_Changed signal; MsgpackSerializer::Deserialize<OnFrameAlignmentPattern_Changed>(buffer, signal); return signal; }
        static idq4p::classes::OnOpticsOptimization_InProgress Decode_OnOpticsOptimization_InProgress(const msgpack::sbuffer& buffer) { OnOpticsOptimization_InProgress signal; MsgpackSerializer::Deserialize<OnOpticsOptimization_InProgress>(buffer, signal); return signal; }*/
    };

} // namespace cqp

#endif // HAVE_IDQ4P
