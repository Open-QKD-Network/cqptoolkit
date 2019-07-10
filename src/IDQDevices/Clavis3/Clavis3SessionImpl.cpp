/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#if defined(HAVE_IDQ4P)

#include "Clavis3SessionImpl.h"
#include <string>

#include "ZmqStringExchange.hpp"
#include "CommandCommunicator.hpp"
#include "Command.hpp"
#include "SignalCommunicator.hpp"
#include <Signal.hpp>
#include "SystemState.hpp"
#include "IntraProcessMessages.hpp"
#include "MsgpackSerializer.hpp"
#include "Commands/GetProtocolVersion.hpp"
#include "Commands/GetSoftwareVersion.hpp"
#include "Commands/GetBoardInformation.hpp"
#include "Commands/UpdateSoftware.hpp"
#include "Commands/PowerOn.hpp"
#include "Commands/SetInitialKey.hpp"
#include "Commands/Zeroize.hpp"
#include "Commands/PowerOff.hpp"
#include "Commands/GetRandomNumber.hpp"
#include "Commands/SubscribeSignal.hpp"
#include "Commands/UnsubscribeSignal.hpp"
#include "Commands/UnsubscribeAllSignals.hpp"
#include "Commands/SetNotificationFrequency.hpp"
#include "Signals/OnSystemState_Changed.hpp"
#include "Signals/FPGA/OnQber_NewValue.hpp"
#include "Signals/FPGA/OnVisibility_NewValue.hpp"
#include "Signals/OnUpdateSoftware_Progress.hpp"
#include "Signals/Alice/OnIM_AmplifierCurrent_AbsoluteOutOfRange.hpp"

#include "ZmqClassExchange.hpp"
#include "QuantumKey.hpp"

#if defined(_DEBUG)
    #include "msgpack.hpp"
#endif
//#include "Algorithms/Util/Hash.h"
#include "Clavis3/Clavis3SignalHandler.h"

namespace cqp
{

    using idq4p::classes::Command;
    using idq4p::domainModel::CommandId;
    using idq4p::domainModel::MessageDirection;
    using idq4p::classes::CommandCommunicator;
    using idq4p::utilities::MsgpackSerializer;
    using idq4p::domainModel::SystemState;

    Clavis3Session::Impl::Impl(const std::string& hostname):
        state(SystemState::NOT_DEFINED)
    {
        try
        {
            using namespace std;
            const std::string prefix {"tcp://"};
#if defined(_DEBUG)
            {
                int zmqVersion[3];
                zmq_version(&zmqVersion[0], &zmqVersion[1], &zmqVersion[2]);
                std::string zmqVersionString = to_string(zmqVersion[0]) + "." +
                                               to_string(zmqVersion[1]) + "." +
                                               to_string(zmqVersion[2]);

                LOGDEBUG("Clavis3 Device created. ZeroMQ Version: " + zmqVersionString + " MsgPack Version: " + msgpack_version());
            }
#endif
            // create a thread to read and process the signals
            signalReader = std::thread(&Impl::ReadSignalSocket, this, prefix + hostname + ":" + std::to_string(signalsPort));

            LOGTRACE("Connecting to management socket");
            mgmtSocket.connect(prefix + hostname + ":" + std::to_string(managementPort));
            mgmtSocket.setsockopt(ZMQ_RCVTIMEO, sockTimeoutMs); // in milliseconds
            mgmtSocket.setsockopt(ZMQ_SNDTIMEO, sockTimeoutMs); // in milliseconds
            mgmtSocket.setsockopt(ZMQ_LINGER, sockTimeoutMs); // Discard pending buffered socket messages on close().
            mgmtSocket.setsockopt(ZMQ_CONNECT_TIMEOUT, sockTimeoutMs);

            SubscribeToSignals();

            LOGTRACE("Connecting to key socket");
            keySocket.connect(prefix + hostname + ":" + std::to_string(keyChannelPort));
            keySocket.setsockopt(ZMQ_SUBSCRIBE, "");
            keySocket.setsockopt(ZMQ_RCVTIMEO, sockTimeoutMs); // in milliseconds

            state = GetCurrentState();
            //LOGINFO("*********** Initial state: " + idq4p::domainModel::SystemState_ToString(state));
            GetProtocolVersion();
            GetSoftwareVersion(SoftwareId::CommunicatorService);
            GetSoftwareVersion(SoftwareId::BoardSupervisorService);
            // clunky way to decide which box it is
            if(GetSoftwareVersion(SoftwareId::RegulatorServiceAlice).GetMajor() >= 0)
            {
                side = remote::Side::Alice;
            }
            else if(GetSoftwareVersion(SoftwareId::RegulatorServiceBob).GetMajor() >= 0)
            {
                side = remote::Side::Bob;
            }
            else
            {
                LOGERROR("Cant work out which side this is");
            }
            GetSoftwareVersion(SoftwareId::FpgaConfiguration);

            LOGTRACE("Device created");
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
    }

    cqp::Clavis3Session::Impl::~Impl()
    {
        shutdown = true;

        keySocket.close();
        mgmtSocket.close();

        if(signalReader.joinable())
        {
            signalReader.join();
        }
        context.close();
    }

    void cqp::Clavis3Session::Impl::PowerOn()
    {
        using namespace idq4p::classes;
        using namespace idq4p::utilities;
        using namespace idq4p::domainModel;

        if(state == SystemState::PowerOff)
        {
            idq4p::classes::PowerOn requestCommnad;
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize<idq4p::classes::PowerOn>(requestCommnad, requestBuffer);
            Command request(CommandId::PowerOn, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::PowerOn, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
        }
        else
        {
            LOGERROR("Cannot perform PowerOn in state " + SystemState_ToString(state));
        }
    }

    idq4p::classes::GetBoardInformation cqp::Clavis3Session::Impl::GetBoardInformation(BoardId whichBoard)
    {
        using idq4p::classes::GetBoardInformation;
        //Serialize request
        GetBoardInformation requestCommand(static_cast<int32_t>(whichBoard));
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<GetBoardInformation>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::GetBoardInformation, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::GetBoardInformation, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        GetBoardInformation boardInfo;
        MsgpackSerializer::Deserialize<GetBoardInformation>(replyBuffer, boardInfo);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + boardInfo.ToString() + ".");
        return boardInfo;
    }

    idq4p::classes::GetSoftwareVersion cqp::Clavis3Session::Impl::GetSoftwareVersion(SoftwareId whichSoftware)
    {
        using idq4p::classes::GetSoftwareVersion;
        //Serialize request
        GetSoftwareVersion requestCommand(static_cast<int32_t>(whichSoftware));
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<GetSoftwareVersion>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::GetSoftwareVersion, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::GetSoftwareVersion, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        GetSoftwareVersion replyCommand;
        MsgpackSerializer::Deserialize<GetSoftwareVersion>(replyBuffer, replyCommand);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
        return replyCommand;
    }

    idq4p::classes::GetProtocolVersion cqp::Clavis3Session::Impl::GetProtocolVersion()
    {
        const idq4p::classes::GetProtocolVersion requestCommand;
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<idq4p::classes::GetProtocolVersion>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::GetProtocolVersion, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::GetProtocolVersion, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        idq4p::classes::GetProtocolVersion replyCommand;
        MsgpackSerializer::Deserialize<idq4p::classes::GetProtocolVersion>(replyBuffer, replyCommand);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
        return replyCommand;
    }

    void cqp::Clavis3Session::Impl::SetInitialKey(const DataBlock& key)
    {
        using idq4p::classes::SetInitialKey;
        using namespace idq4p::utilities;
        using namespace idq4p::domainModel;

        if(state == SystemState::ExecutingSecurityInitialization)
        {
            // Send request
            SetInitialKey requestCommand(key);
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize<SetInitialKey>(requestCommand, requestBuffer);
            const Command request(CommandId::SetInitialKey, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::SetInitialKey, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "' " + requestCommand.ToString() + ".");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            // Deserialize reply
            msgpack::sbuffer replyBuffer;
            reply.GetBuffer(replyBuffer);
            SetInitialKey replyCommand;
            MsgpackSerializer::Deserialize<SetInitialKey>(replyBuffer, replyCommand);
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
        }
        else
        {
            LOGERROR("Cannot perform SetInitialKey in state " + SystemState_ToString(state));
        }
    }

    bool Clavis3Session::Impl::GetRandomNumber(std::vector<uint8_t>& out)
    {
        bool result = false;
        using namespace idq4p::domainModel;
        using idq4p::classes::GetRandomNumber;
        if(state == SystemState::ExecutingGeneralInitialization ||
                state == SystemState::ExecutingSecurityInitialization ||
                state == SystemState::Running ||
                state == SystemState::HandlingError)
        {
            // Send request
            GetRandomNumber requestCommand(16);
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize<GetRandomNumber>(requestCommand, requestBuffer);
            const Command request(CommandId::GetRandomNumber, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::GetRandomNumber, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "' " + requestCommand.ToString() + ".");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            // Deserialize reply
            msgpack::sbuffer replyBuffer;
            reply.GetBuffer(replyBuffer);
            GetRandomNumber replyCommand;
            MsgpackSerializer::Deserialize<GetRandomNumber>(replyBuffer, replyCommand);
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");

            out = replyCommand.GetNumber();
            result = replyCommand.GetState() == 1;
        }
        else
        {
            LOGERROR("Cannot perform GetRandomNumber in state " + SystemState_ToString(state));
        }
        return result;
    }

    void cqp::Clavis3Session::Impl::Zeroize()
    {
        using idq4p::classes::Zeroize;

        // Send request
        const Command request(CommandId::Zeroize, MessageDirection::Request);
        Command reply(  CommandId::Zeroize, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    void cqp::Clavis3Session::Impl::UpdateSoftware(const std::string& filename, const std::string& filenameSha1)
    {
        using namespace idq4p::domainModel;
        using idq4p::classes::UpdateSoftware;
        if(state == SystemState::ExecutingSelfTest ||
                state == SystemState::ExecutingGeneralInitialization ||
                state == SystemState::ExecutingSecurityInitialization ||
                state == SystemState::Running)
        {
            //Serialize request
            UpdateSoftware requestCommand(5, filename, filenameSha1);
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize<UpdateSoftware>(requestCommand, requestBuffer);
            // Send request
            const Command request(CommandId::UpdateSoftware, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::UpdateSoftware, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "' " + requestCommand.ToString() + ".");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            // Deserialize reply
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
        }
        else
        {
            LOGERROR("Cannot perform UpdateSoftware in state " + SystemState_ToString(state));
        }
    }

    void cqp::Clavis3Session::Impl::PowerOff()
    {
        if(!shutdown &&
                state != SystemState::PowerOff &&
                state != SystemState::UpdatingSoftware &&
                state != SystemState::Zeroizing &&
                state != SystemState::PoweringOff)
        {
            idq4p::classes::PowerOff requestCommand;
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize(requestCommand, requestBuffer);
            const Command request(CommandId::PowerOff, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::PowerOff, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
        }
        else
        {
            LOGERROR("Cannot perform PowerOff in state " + SystemState_ToString(state));
        }
    }

    void cqp::Clavis3Session::Impl::Reboot()
    {
        if(!shutdown  &&
                state != SystemState::PowerOff &&
                state != SystemState::UpdatingSoftware &&
                state != SystemState::Zeroizing &&
                state != SystemState::PoweringOff)
        {
            idq4p::classes::PowerOff requestCommand;
            msgpack::sbuffer requestBuffer;
            MsgpackSerializer::Serialize(requestCommand, requestBuffer);
            const Command request(CommandId::Restart, MessageDirection::Request, requestBuffer);
            Command reply(  CommandId::Restart, MessageDirection::Reply);
            LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
            CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
            LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
        }
        else
        {
            LOGERROR("Cannot perform Reboot in state " + SystemState_ToString(state));
        }
    }

    void Clavis3Session::Impl::SubscribeToSignals()
    {
        using namespace idq4p::domainModel;

        std::vector<SignalId> subscribeTo
        {
            SignalId::OnSystemState_Changed,
            SignalId::OnPowerupComponentsState_Changed,
            SignalId::OnAlignmentState_Changed,
            SignalId::OnOptimizingOpticsState_Changed,
            SignalId::OnShutdownState_Changed,
            SignalId::OnQber_NewValue,
            SignalId::OnVisibility_NewValue,
            SignalId::OnOpticsOptimization_InProgress
        };
        /*
                if(side == remote::Side_Type::Side_Type_Alice)
                {
                    subscribeTo.push_back(SignalId::OnLaser_Temperature_NewValue);
                    subscribeTo.push_back(SignalId::OnIM_Temperature_NewValue);
                }
                else if(side == remote::Side_Type::Side_Type_Bob)
                {
                    subscribeTo.push_back(SignalId::OnIF_Temperature_NewValue);
                    subscribeTo.push_back(SignalId::OnFilter_Temperature_NewValue);
                    subscribeTo.push_back(SignalId::OnDataDetector_Temperature_NewValue);
                    subscribeTo.push_back(SignalId::OnMonitorDetector_Temperature_NewValue);
                }
        */

        for(const auto sig : subscribeTo)
        {
            SubscribeToSignal(sig);
            SetNotificationFrequency(sig, signalRate);
        }

        // block some of the noise that keeps comming out
        std::vector<SignalId> unsubscribe
        {
            SignalId::OnIF_Temperature_NewValue,
            SignalId::OnDataDetector_BiasCurrent_NewValue,
            SignalId::OnMonitorDetector_BiasCurrent_NewValue,
            SignalId::OnIM_AmplifierCurrent_NewValue,
            SignalId::OnIM_AmplifierVoltage_NewValue,
            SignalId::OnIM_Temperature_NewValue,
            SignalId::OnIM_TECCurrent_NewValue,
            SignalId::OnVOA_Attenuation_NewValue,
            SignalId::OnIM_BiasVoltage_AbsoluteOutOfRange,
            SignalId::OnMonitoringPhotodiode_Power_OperationOutOfRange
        };

        for(const auto sig : subscribeTo)
        {
            // BUG: This doesn't work so slow them down
            UnsubscribeSignal(sig);
            SetNotificationFrequency(sig, signalRate);
        }

    }

    void Clavis3Session::Impl::SetNotificationFrequency(idq4p::domainModel::SignalId sigId, float rateHz)
    {
        using idq4p::classes::SetNotificationFrequency;
        SetNotificationFrequency requestCommand(static_cast<uint32_t>(sigId), rateHz);
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<SetNotificationFrequency>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::SetNotificationFrequency, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::SetNotificationFrequency, MessageDirection::Reply);

        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    idq4p::domainModel::SystemState Clavis3Session::Impl::GetCurrentState()
    {
        idq4p::classes::OnSystemState_Changed requestCommand;
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize(requestCommand, requestBuffer);
        const Command request(CommandId::GetSystemState, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::GetSystemState, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        idq4p::classes::OnSystemState_Changed replyCommand;
        MsgpackSerializer::Deserialize<idq4p::classes::OnSystemState_Changed>(replyBuffer, replyCommand);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
        return replyCommand.GetState();
    }

    void Clavis3Session::Impl::SubscribeToSignal(idq4p::domainModel::SignalId sig)
    {
        using idq4p::classes::SubscribeSignal;
        //Serialize request
        SubscribeSignal requestCommand(static_cast<uint32_t>(sig));
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<SubscribeSignal>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::SubscribeSignal, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::SubscribeSignal, MessageDirection::Reply);

        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    void Clavis3Session::Impl::UnsubscribeSignal(idq4p::domainModel::SignalId sig)
    {
        using idq4p::classes::UnsubscribeSignal;
        //Serialize request
        UnsubscribeSignal requestCommand(static_cast<uint32_t>(sig));
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<UnsubscribeSignal>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::UnsubscribeSignal, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::UnsubscribeSignal, MessageDirection::Reply);

        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    bool Clavis3Session::Impl::ReadKey(PSK& keyValue)
    {
        using namespace idq4p::classes;
        using namespace idq4p::domainModel;
        using namespace idq4p::utilities;

        bool result = false;
        //Wait for key from QKE
        QuantumKey key;
        try
        {
            ZmqClassExchange::Receive<QuantumKey>(keySocket, key);

            LOGINFO("KeyChannel: received '" + key.ToString() + "'");

            //id = FNV1aHash(key.GetId());
            keyValue = key.GetKeyValue();
            result = true;
        }
        catch(const std::runtime_error&)
        {
            // call was probably canceled due to closing socket
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
        return result;
    }

    remote::Side::Type Clavis3Session::Impl::GetSide() const
    {
        return side;
    }

    idq4p::domainModel::SystemState Clavis3Session::Impl::GetState()
    {
        return state;
    }

    LogLevel SignalToErrorLevel(domo::SeverityId severity)
    {
        LogLevel result = LogLevel::Silent;
        switch(severity)
        {
        case domo::SeverityId::Info:
            result = LogLevel::Info;
            break;
        case domo::SeverityId::Debug:
            result = LogLevel::Debug;
            break;
        case domo::SeverityId::Fatal:
        case domo::SeverityId::Error:
            result = LogLevel::Error;
            break;
        case domo::SeverityId::Warning:
            result = LogLevel::Warning;
            break;
        case domo::SeverityId::NOT_DEFINED:
            result = LogLevel::Silent;
        }
        return result;
    }

    void Clavis3Session::Impl::ReadSignalSocket(const std::string& address)
    {
        using namespace idq4p::classes;
        using namespace idq4p::domainModel;

        zmq::socket_t signalSocket{context, ZMQ_SUB};
        LOGTRACE("Connecting to signal socket");
        signalSocket.connect(address);
        signalSocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        signalSocket.setsockopt(ZMQ_RCVTIMEO, sockTimeoutMs); // in milliseconds
        signalSocket.setsockopt(ZMQ_SNDTIMEO, sockTimeoutMs); // in milliseconds
        signalSocket.setsockopt(ZMQ_LINGER, sockTimeoutMs); // Discard pending buffered socket messages on close().
        signalSocket.setsockopt(ZMQ_CONNECT_TIMEOUT, sockTimeoutMs);

        while(!shutdown && signalSocket.connected())
        {
            Signal signalWrapper;
            try
            {
                // wait for message
                SignalCommunicator::Receive(signalSocket, signalWrapper);
                // get the data from the message
                msgpack::sbuffer buffer;
                signalWrapper.GetBuffer(buffer);

                // handle signals we're interested in
                switch (signalWrapper.GetId())
                {
                case SignalId::NOT_DEFINED:
                    break;
                //Software Signals
                case SignalId::OnSystemState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnSystemState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), "==========" + signal.ToString() + "==========");
                    state = signal.GetState();
                }
                break;
                case SignalId::OnUpdateSoftware_Progress:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnUpdateSoftware_Progress(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    if(signal.GetProgress() == 100)
                    {
                        LOGINFO("Software update complete, please power cycle the device");
                        UnsubscribeSignal(SignalId::OnUpdateSoftware_Progress);
                    }
                }
                break;
                case SignalId::OnPowerupComponentsState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnPowerupComponentsState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnAlignmentState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnAlignmentState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnOptimizingOpticsState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnOptimizingOpticsState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnShutdownState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnShutdownState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnKeySecurity_OutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnKeySecurity_OutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnKeyAuthentication_Mismatch:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnKeyAuthentication_Mismatch(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnKeySecurity_SingleFailure:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnKeySecurity_SingleFailure(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnKeySecurity_RepeatedFailure:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnKeySecurity_RepeatedFailure(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnKeyDeliverer_Exception:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnKeyDeliverer_Exception(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnCommandServer_Exception:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnCommandServer_Exception(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                //Alice
                case SignalId::OnLaser_BiasCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_BiasCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_BiasCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_BiasCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Power_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Power_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Power_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Power_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_Power_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_Power_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_TECCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_TECCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_TECCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_TECCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnLaser_TECCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnLaser_TECCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_BiasVoltage_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_BiasVoltage_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_BiasVoltage_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_BiasVoltage_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_BiasVoltage_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_BiasVoltage_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierVoltage_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierVoltage_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_AmplifierVoltage_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_AmplifierVoltage_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_TECCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_TECCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_TECCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_TECCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIM_TECCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIM_TECCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnVOA_Attenuation_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnVOA_Attenuation_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnVOA_Attenuation_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnVOA_Attenuation_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitoringPhotodiode_Power_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitoringPhotodiode_Power_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitoringPhotodiode_Power_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitoringPhotodiode_Power_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                //Bob
                case SignalId::OnIF_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIF_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIF_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIF_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnIF_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnIF_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnFilter_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnFilter_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnFilter_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnFilter_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnFilter_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnFilter_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_BiasCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_BiasCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnDataDetector_BiasCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnDataDetector_BiasCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_Temperature_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_Temperature_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_Temperature_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_Temperature_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_BiasCurrent_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_AbsoluteOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                case SignalId::OnMonitorDetector_BiasCurrent_OperationOutOfRange:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnMonitorDetector_BiasCurrent_OperationOutOfRange(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                //FPGA
                case SignalId::OnQber_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnQber_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    errorStats.QBER.Update(static_cast<double>(signal.GetValue()));
                }
                break;
                case SignalId::OnVisibility_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnVisibility_NewValue(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    alignementStats.visibility.Update(static_cast<double>(signal.GetValue()));
                }
                break;
                case SignalId::OnFPGA_Failure:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnFPGA_Failure(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;

                // others:
                case SignalId::OnOpticsOptimization_InProgress:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnOptimizingOpticsState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                }
                break;
                default:
                    LOGERROR("Unknown signal: " + signalWrapper.ToString());
                    break;
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // while

        signalSocket.close();
    } // ReadSignalSocket

} // namespace cqp
#endif //HAVE_IDQ4P
