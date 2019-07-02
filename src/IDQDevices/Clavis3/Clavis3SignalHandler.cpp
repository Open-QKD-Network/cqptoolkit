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
#include "Clavis3SignalHandler.h"

using namespace idq4p::classes;
using namespace idq4p::utilities;

namespace cqp
{

    Clavis3SignalHandler::Clavis3SignalHandler()
    {

    }

    OnSystemState_Changed Clavis3SignalHandler::Decode_OnSystemState_Changed(const msgpack::sbuffer& buffer)
    {
        OnSystemState_Changed signal;
        MsgpackSerializer::Deserialize<OnSystemState_Changed>(buffer, signal);
        return signal;
    }

    OnUpdateSoftware_Progress Clavis3SignalHandler::Decode_OnUpdateSoftware_Progress(const msgpack::sbuffer& buffer)
    {
        OnUpdateSoftware_Progress signal;
        MsgpackSerializer::Deserialize<OnUpdateSoftware_Progress>(buffer, signal);
        return signal;
    }

    OnPowerUpComponentsState_Changed Clavis3SignalHandler::Decode_OnPowerupComponentsState_Changed(const msgpack::sbuffer& buffer)
    {
        OnPowerUpComponentsState_Changed signal;
        MsgpackSerializer::Deserialize<OnPowerUpComponentsState_Changed>(buffer, signal);
        return signal;
    }

    OnAlignmentState_Changed Clavis3SignalHandler::Decode_OnAlignmentState_Changed(const msgpack::sbuffer& buffer)
    {
        OnAlignmentState_Changed signal;
        MsgpackSerializer::Deserialize<OnAlignmentState_Changed>(buffer, signal);
        return signal;
    }

    OnOptimizingOpticsState_Changed Clavis3SignalHandler::Decode_OnOptimizingOpticsState_Changed(const msgpack::sbuffer& buffer)
    {
        OnOptimizingOpticsState_Changed signal;
        MsgpackSerializer::Deserialize<OnOptimizingOpticsState_Changed>(buffer, signal);
        return signal;
    }

    OnShutdownState_Changed Clavis3SignalHandler::Decode_OnShutdownState_Changed(const msgpack::sbuffer& buffer)
    {
        OnShutdownState_Changed signal;
        MsgpackSerializer::Deserialize<OnShutdownState_Changed>(buffer, signal);
        return signal;
    }

    OnKeySecurity_OutOfRange Clavis3SignalHandler::Decode_OnKeySecurity_OutOfRange(const msgpack::sbuffer& buffer)
    {
        OnKeySecurity_OutOfRange signal;
        MsgpackSerializer::Deserialize<OnKeySecurity_OutOfRange>(buffer, signal);
        return signal;
    }

    OnKeyAuthentication_Mismatch Clavis3SignalHandler::Decode_OnKeyAuthentication_Mismatch(const msgpack::sbuffer& buffer)
    {
        OnKeyAuthentication_Mismatch signal;
        MsgpackSerializer::Deserialize<OnKeyAuthentication_Mismatch>(buffer, signal);
        return signal;
    }

    OnKeySecurity_SingleFailure Clavis3SignalHandler::Decode_OnKeySecurity_SingleFailure(const msgpack::sbuffer& buffer)
    {
        OnKeySecurity_SingleFailure signal;
        MsgpackSerializer::Deserialize<OnKeySecurity_SingleFailure>(buffer, signal);
        return signal;
    }

    OnKeySecurity_RepeatedFailure Clavis3SignalHandler::Decode_OnKeySecurity_RepeatedFailure(const msgpack::sbuffer& buffer)
    {
        OnKeySecurity_RepeatedFailure signal;
        MsgpackSerializer::Deserialize<OnKeySecurity_RepeatedFailure>(buffer, signal);
        return signal;
    }

    OnKeyDeliverer_Exception Clavis3SignalHandler::Decode_OnKeyDeliverer_Exception(const msgpack::sbuffer& buffer)
    {
        OnKeyDeliverer_Exception signal;
        MsgpackSerializer::Deserialize<OnKeyDeliverer_Exception>(buffer, signal);
        return signal;
    }

    OnCommandServer_Exception Clavis3SignalHandler::Decode_OnCommandServer_Exception(const msgpack::sbuffer& buffer)
    {
        OnCommandServer_Exception signal;
        MsgpackSerializer::Deserialize<OnCommandServer_Exception>(buffer, signal);
        return signal;
    }

    OnLaser_BiasCurrent_NewValue Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnLaser_BiasCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnLaser_BiasCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnLaser_BiasCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_BiasCurrent_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_BiasCurrent_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_BiasCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_BiasCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_BiasCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_Temperature_NewValue Clavis3SignalHandler::Decode_OnLaser_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnLaser_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnLaser_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnLaser_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnLaser_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_Temperature_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_Temperature_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnLaser_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_Power_NewValue Clavis3SignalHandler::Decode_OnLaser_Power_NewValue(const msgpack::sbuffer& buffer)
    {
        OnLaser_Power_NewValue signal;
        MsgpackSerializer::Deserialize<OnLaser_Power_NewValue>(buffer, signal);
        return signal;
    }

    OnLaser_Power_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnLaser_Power_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_Power_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_Power_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_Power_OperationOutOfRange Clavis3SignalHandler::Decode_OnLaser_Power_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_Power_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_Power_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_TECCurrent_NewValue Clavis3SignalHandler::Decode_OnLaser_TECCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnLaser_TECCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnLaser_TECCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnLaser_TECCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnLaser_TECCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_TECCurrent_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_TECCurrent_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnLaser_TECCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnLaser_TECCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnLaser_TECCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnLaser_TECCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_BiasVoltage_NewValue Clavis3SignalHandler::Decode_OnIM_BiasVoltage_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIM_BiasVoltage_NewValue signal;
        MsgpackSerializer::Deserialize<OnIM_BiasVoltage_NewValue>(buffer, signal);
        return signal;
    }

    OnIM_BiasVoltage_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIM_BiasVoltage_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_BiasVoltage_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_BiasVoltage_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_BiasVoltage_OperationOutOfRange Clavis3SignalHandler::Decode_OnIM_BiasVoltage_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_BiasVoltage_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_BiasVoltage_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierCurrent_NewValue Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierCurrent_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierCurrent_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierVoltage_NewValue Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierVoltage_NewValue signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierVoltage_NewValue>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierVoltage_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierVoltage_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierVoltage_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_AmplifierVoltage_OperationOutOfRange Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_AmplifierVoltage_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_AmplifierVoltage_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_Temperature_NewValue Clavis3SignalHandler::Decode_OnIM_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIM_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnIM_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnIM_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIM_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_Temperature_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_Temperature_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnIM_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_TECCurrent_NewValue Clavis3SignalHandler::Decode_OnIM_TECCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIM_TECCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnIM_TECCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnIM_TECCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIM_TECCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_TECCurrent_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_TECCurrent_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIM_TECCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnIM_TECCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIM_TECCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIM_TECCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnVOA_Attenuation_NewValue Clavis3SignalHandler::Decode_OnVOA_Attenuation_NewValue(const msgpack::sbuffer& buffer)
    {
        OnVOA_Attenuation_NewValue signal;
        MsgpackSerializer::Deserialize<OnVOA_Attenuation_NewValue>(buffer, signal);
        return signal;
    }

    OnVOA_Attenuation_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnVOA_Attenuation_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnVOA_Attenuation_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnVOA_Attenuation_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnMonitoringPhotodiode_Power_NewValue Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_NewValue(const msgpack::sbuffer& buffer)
    {
        OnMonitoringPhotodiode_Power_NewValue signal;
        MsgpackSerializer::Deserialize<OnMonitoringPhotodiode_Power_NewValue>(buffer, signal);
        return signal;
    }

    OnMonitoringPhotodiode_Power_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitoringPhotodiode_Power_AbsoluteOutOfRange  signal;
        MsgpackSerializer::Deserialize<OnMonitoringPhotodiode_Power_AbsoluteOutOfRange >(buffer, signal);
        return signal;
    }

    OnMonitoringPhotodiode_Power_OperationOutOfRange Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitoringPhotodiode_Power_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnMonitoringPhotodiode_Power_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnIF_Temperature_NewValue Clavis3SignalHandler::Decode_OnIF_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnIF_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnIF_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnIF_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnIF_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIF_Temperature_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIF_Temperature_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnIF_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnIF_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnIF_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnIF_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnFilter_Temperature_NewValue Clavis3SignalHandler::Decode_OnFilter_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnFilter_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnFilter_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnFilter_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnFilter_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnFilter_Temperature_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnFilter_Temperature_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnFilter_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnFilter_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnFilter_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnFilter_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnDataDetector_Temperature_NewValue Clavis3SignalHandler::Decode_OnDataDetector_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnDataDetector_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnDataDetector_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnDataDetector_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_Temperature_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnDataDetector_Temperature_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnDataDetector_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnDataDetector_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnDataDetector_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnDataDetector_BiasCurrent_NewValue Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_BiasCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnDataDetector_BiasCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnDataDetector_BiasCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_BiasCurrent_AbsoluteOutOfRange signal;
        MsgpackSerializer::Deserialize<OnDataDetector_BiasCurrent_AbsoluteOutOfRange>(buffer, signal);
        return signal;
    }

    OnDataDetector_BiasCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnDataDetector_BiasCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnDataDetector_BiasCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnMonitorDetector_Temperature_NewValue Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_NewValue(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_Temperature_NewValue signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_Temperature_NewValue>(buffer, signal);
        return signal;
    }

    OnMonitorDetector_Temperature_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_Temperature_AbsoluteOutOfRange  signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_Temperature_AbsoluteOutOfRange >(buffer, signal);
        return signal;
    }

    OnMonitorDetector_Temperature_OperationOutOfRange Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_Temperature_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_Temperature_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnMonitorDetector_BiasCurrent_NewValue Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_NewValue(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_BiasCurrent_NewValue signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_BiasCurrent_NewValue>(buffer, signal);
        return signal;
    }

    OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange  signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange >(buffer, signal);
        return signal;
    }

    OnMonitorDetector_BiasCurrent_OperationOutOfRange Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_OperationOutOfRange(const msgpack::sbuffer& buffer)
    {
        OnMonitorDetector_BiasCurrent_OperationOutOfRange signal;
        MsgpackSerializer::Deserialize<OnMonitorDetector_BiasCurrent_OperationOutOfRange>(buffer, signal);
        return signal;
    }

    OnQber_NewValue Clavis3SignalHandler::Decode_OnQber_NewValue(const msgpack::sbuffer& buffer)
    {
        OnQber_NewValue signal;
        MsgpackSerializer::Deserialize<OnQber_NewValue>(buffer, signal);
        return signal;
    }

    OnVisibility_NewValue Clavis3SignalHandler::Decode_OnVisibility_NewValue(const msgpack::sbuffer& buffer)
    {
        OnVisibility_NewValue signal;
        MsgpackSerializer::Deserialize<OnVisibility_NewValue>(buffer, signal);
        return signal;
    }

    OnFPGA_Failure Clavis3SignalHandler::Decode_OnFPGA_Failure(const msgpack::sbuffer& buffer)
    {
        OnFPGA_Failure signal;
        MsgpackSerializer::Deserialize<OnFPGA_Failure>(buffer, signal);
        return signal;
    }

} // namespace cqp
