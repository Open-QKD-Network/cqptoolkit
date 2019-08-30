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
#include "Algorithms/Util/Hash.h"
#include "Clavis3/Clavis3SignalHandler.h"
#include "QKDInterfaces/ISync.grpc.pb.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include <thread>

namespace cqp
{

    using idq4p::classes::Command;
    using idq4p::domainModel::CommandId;
    using idq4p::domainModel::MessageDirection;
    using idq4p::classes::CommandCommunicator;
    using idq4p::utilities::MsgpackSerializer;
    using idq4p::domainModel::SystemState;

    Clavis3Session::Impl::Impl(const std::string& hostname):
        deviceAddress(hostname),
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

            LOGTRACE("Connecting to key socket");
            keySocket.connect(prefix + hostname + ":" + std::to_string(keyChannelPort));
            keySocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
            keySocket.setsockopt(ZMQ_RCVTIMEO, sockTimeoutMs); // in milliseconds
            keySocket.setsockopt(ZMQ_LINGER, sockTimeoutMs); // Discard pending buffered socket messages on close().

            state = GetCurrentState();
            LOGINFO("*********** Initial state: " + idq4p::domainModel::SystemState_ToString(state));
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

    void cqp::Clavis3Session::Impl::SendInitialKey(const DataBlock& key)
    {
        using idq4p::classes::SetInitialKey;
        using namespace idq4p::utilities;
        using namespace idq4p::domainModel;

        if(state == SystemState::PowerOff ||
                state == SystemState::ExecutingGeneralInitialization ||
                state == SystemState::ExecutingSecurityInitialization)
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
        idq4p::classes::Zeroize requestCommand;
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<idq4p::classes::Zeroize>(requestCommand, requestBuffer);
        const Command request(CommandId::Zeroize, MessageDirection::Request, requestBuffer);
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
            // values taken from cockpit software initialisation
            SignalId::OnSystemState_Changed,
            SignalId::OnUpdateSoftware_Progress,
            SignalId::OnPowerupComponentsState_Changed,
            SignalId::OnAlignmentState_Changed,
            SignalId::OnOptimizingOpticsState_Changed,
            SignalId::OnShutdownState_Changed,
            SignalId::OnLaser_BiasCurrent_NewValue,
            SignalId::OnIM_BiasVoltage_NewValue,
            SignalId::OnLaser_Temperature_NewValue,
            SignalId::OnLaser_Power_NewValue,
            SignalId::OnLaser_TECCurrent_NewValue,
            SignalId::OnIM_AmplifierCurrent_NewValue,
            SignalId::OnIM_AmplifierVoltage_NewValue,
            SignalId::OnIM_Temperature_NewValue,
            SignalId::OnIM_TECCurrent_NewValue,
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
        /*
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
        */
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

    bool Clavis3Session::Impl::ReadKeys(ClavisKeyList& keys)
    {
        LOGTRACE("");
        using namespace idq4p::classes;
        using namespace idq4p::domainModel;
        using namespace idq4p::utilities;

        bool result = false;
        //Wait for key from QKE
        QuantumKey key;
        try
        {
            //HACK
            state = GetCurrentState();
            LOGINFO("*********** Current state: " + idq4p::domainModel::SystemState_ToString(state));
            zmq::message_t msgRequest;

            if(keySocket.recv(&msgRequest))
            {
                // try to prepare the buffer for the number of keys that will arrive
                keys.reserve(averageKeysPerBurst);

                // if we had a message theres probably more, keep getting until there are none left
                do
                {
                    MsgpackSerializer::Deserialize(msgRequest, key);
                    LOGINFO("KeyChannel: received '" + key.ToString() + "'");

                    //id = FNV1aHash(key.GetId());
                    keys.emplace_back(UUID(key.GetId()), key.GetKeyValue());
                    result = true;
                }
                while(keySocket.recv(&msgRequest, ZMQ_DONTWAIT));
                // update the average keys, limiting it to something sensible
                averageKeysPerBurst = std::min(maxKeysPerBurst, 1 + (averageKeysPerBurst + keys.size()) / 2);
            }
        }
        catch(const std::runtime_error&)
        {
            // call was probably canceled due to closing socket
            LOGDEBUG("Non fatal runtime error, socket closed");
        }
        catch(const std::exception& e)
        {
            LOGERROR("Error reading keys: " + e.what());
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

    void Clavis3Session::Impl::SetInitialKey(std::unique_ptr<PSK> newInitailKey)
    {
        this->initialKey = move(newInitailKey);
        if(initialKey && initialKey->size() < requiredInitialKeySize)
        {
            initialKey->insert(initialKey->end(), defaultInitialKey.begin(), defaultInitialKey.begin() + (requiredInitialKeySize - initialKey->size()));
        }
        else if(initialKey && initialKey->size() > requiredInitialKeySize)
        {
            initialKey->resize(requiredInitialKeySize);
        }
    }

    void Clavis3Session::Impl::SendInitialKey()
    {
        state = GetState();
        while(state != idq4p::domainModel::SystemState::ExecutingSecurityInitialization)
        {
            LOGINFO("Waiting to send initial key...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            state = GetState();
        }

        if(!initialKey)
        {
            SendInitialKey(defaultInitialKey);
        }
        else
        {
            // send the initial key
            SendInitialKey(*initialKey);
        }

        state = GetState();
    }

    void Clavis3Session::Impl::SetBobChannel(std::shared_ptr<grpc::Channel> channel)
    {
        bobChannel = channel;
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

    void Clavis3Session::Impl::OnSystemStateChanged(domo::SystemState state)
    {
        if(state == domo::SystemState::ExecutingSecurityInitialization)
        {
            if(!initialKey)
            {
                SendInitialKey(defaultInitialKey);
            }
            else
            {
                // send the initial key
                SendInitialKey(*initialKey);
            }
            auto bob = remote::ISync::NewStub(bobChannel);

            grpc::ClientContext ctx;
            google::protobuf::Empty response;
            LogStatus(bob->SendInitialKey(&ctx, google::protobuf::Empty(), &response));
        }
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
                LOGTRACE("Got message id: " +
                         std::to_string(static_cast<int32_t>(signalWrapper.GetId())) + " = " +
                         SignalId_ToString(signalWrapper.GetId()));

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
                    clavis3Stats.SystemState_Changed.Update(static_cast<size_t>(signal.GetState()));
                    OnSystemStateChanged(signal.GetState());
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
                    clavis3Stats.PowerupComponentsState_Changed.Update(static_cast<size_t>(signal.GetState()));
                }
                break;
                case SignalId::OnAlignmentState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnAlignmentState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.AlignmentState_Changed.Update(static_cast<size_t>(signal.GetState()));
                }
                break;
                case SignalId::OnOptimizingOpticsState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnOptimizingOpticsState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.OptimizingOpticsState_Changed.Update(static_cast<size_t>(signal.GetState()));
                }
                break;
                case SignalId::OnShutdownState_Changed:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnShutdownState_Changed(buffer);
                    cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.ShutdownState_Changed.Update(static_cast<size_t>(signal.GetState()));
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.Laser_BiasCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.Laser_Temperature.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.Laser_Power.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.Laser_TECCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IM_BiasVoltage.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IM_AmplifierCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IM_AmplifierVoltage.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IM_Temperature.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IM_TECCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.VOA_Attenuation.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.MonitoringPhotodiode_Power.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.IF_Temperature.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.Filter_Temperature.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.DataDetector_Temperature.Update(signal.GetValue());
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
                    clavis3Stats.DataDetector_BiasCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.MonitorDetector_Temperature.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    clavis3Stats.MonitorDetector_BiasCurrent.Update(signal.GetValue());
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
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    errorStats.QBER.Update(static_cast<double>(signal.GetValue()));
                    clavis3Stats.Qber.Update(signal.GetValue());
                }
                break;
                case SignalId::OnVisibility_NewValue:
                {
                    const auto signal = Clavis3SignalHandler::Decode_OnVisibility_NewValue(buffer);
                    //cqp::DefaultLogger().Log(SignalToErrorLevel(signal.GetSeverity()), signal.ToString());
                    alignementStats.visibility.Update(static_cast<double>(signal.GetValue()));
                    clavis3Stats.Visibility.Update(signal.GetValue());
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
                    clavis3Stats.OptimizingOpticsState_Changed.Update(static_cast<size_t>(signal.GetState()));
                }
                break;
                case SignalId::OnTimebinAlignmentPattern_Changed:
                {
                    LOGWARN("Recieved OnTimebinAlignmentPattern_Changed but dont know how to decode it");
                }
                break;
                default:
                    LOGERROR("Unknown signal: " + signalWrapper.ToString());
                    break;
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR("Failed to decode message: " + e.what());
            }
        } // while

        signalSocket.close();
    }

    // ReadSignalSocket

    const PSK Clavis3Session::Impl::defaultInitialKey{{
            0x72, 0x2e, 0xc9, 0x44, 0x3c, 0x1c, 0x9a, 0x03, 0x21, 0x17, 0x9f, 0xff, 0xd8, 0x88, 0x7f, 0x3e,
            0x42, 0xab, 0x27, 0x59, 0xa3, 0x87, 0x62, 0xc6, 0xaa, 0x4f, 0xa7, 0x3f, 0xea, 0x7b, 0xa4, 0xa7,
            0x9e, 0x18, 0x35, 0x9c, 0xaf, 0x18, 0x7d, 0x7c, 0xfb, 0xeb, 0x4a, 0xe1, 0x3d, 0x50, 0x83, 0x19,
            0xb2, 0x07, 0x3f, 0x92, 0x8b, 0xe9, 0x0a, 0x43, 0x8f, 0x0f, 0xe5, 0xb3, 0xdc, 0x87, 0xef, 0x65,
            0x14, 0x95, 0x41, 0xed, 0xcc, 0x52, 0x80, 0x47, 0x46, 0x49, 0xcc, 0x1d, 0x00, 0xa2, 0x6c, 0xf8,
            0x57, 0xc1, 0xdf, 0x5e, 0x08, 0x7c, 0xca, 0xd4, 0x7b, 0x84, 0xd9, 0xa2, 0xb6, 0x4c, 0x49, 0x95,
            0xfd, 0x2d, 0xe6, 0x35, 0xbe, 0x61, 0xd6, 0x50, 0xd4, 0x89, 0x4e, 0x37, 0x57, 0xf4, 0x69, 0x59,
            0xf7, 0x16, 0xda, 0xf2, 0x37, 0x76, 0xcb, 0x3d, 0x9a, 0xf2, 0x24, 0xdd, 0xeb, 0x87, 0xb6, 0xd5,
            0x58, 0x68, 0x7c, 0xff, 0x77, 0xf5, 0x49, 0x99, 0x55, 0x3f, 0x8f, 0x13, 0x2f, 0x7d, 0xb9, 0x3e,
            0x9e, 0xc4, 0xc3, 0x0e, 0x80, 0xa1, 0x68, 0x5a, 0xbd, 0x4d, 0x6d, 0x01, 0xef, 0x50, 0xfd, 0x47,
            0xbc, 0xbd, 0x2e, 0xeb, 0xa0, 0x65, 0xf3, 0x53, 0xd7, 0xeb, 0xa2, 0xf6, 0x36, 0xe2, 0xbc, 0xf0,
            0x97, 0xb6, 0xa7, 0x3c, 0xa0, 0xf4, 0x48, 0x9e, 0x76, 0x0a, 0x0d, 0xbe, 0x8c, 0xce, 0x42, 0x3c,
            0x0e, 0xc9, 0x57, 0xaf, 0x9e, 0xe3, 0x74, 0x6c, 0xcd, 0xe8, 0xba, 0x3b, 0xc4, 0xf7, 0x57, 0x5d,
            0x25, 0xca, 0x7c, 0x10, 0x82, 0xa0, 0xdd, 0xd2, 0x7e, 0x2c, 0xab, 0x64, 0xfb, 0x1c, 0x5d, 0x49,
            0x6c, 0x0c, 0x26, 0x43, 0xe2, 0x76, 0x9b, 0x6e, 0x62, 0x2b, 0xc7, 0x1b, 0xa8, 0xad, 0x38, 0x54,
            0x2b, 0x14, 0x2d, 0xf1, 0x8a, 0x1b, 0x08, 0x3c, 0xad, 0x09, 0x13, 0xac, 0xaa, 0x85, 0xcf, 0x68,
            0x13, 0xb1, 0xc1, 0x88, 0x7b, 0x39, 0x47, 0x76, 0xe3, 0xda, 0x52, 0x79, 0x26, 0x83, 0xce, 0x1e,
            0x4c, 0xa4, 0x11, 0xb8, 0x82, 0xb3, 0xee, 0xbc, 0xec, 0x30, 0xc9, 0xe4, 0xbf, 0xe7, 0x38, 0x73,
            0x82, 0x53, 0x54, 0x19, 0x69, 0x49, 0xc8, 0x15, 0xbd, 0x85, 0xa1, 0x52, 0xb1, 0xef, 0x27, 0x41,
            0x09, 0x65, 0xd8, 0x29, 0xe9, 0xd9, 0xb5, 0x82, 0xca, 0x45, 0x89, 0x00, 0xed, 0x17, 0x82, 0xda,
            0x91, 0x27, 0x63, 0xda, 0xd3, 0xb0, 0x8e, 0x15, 0x32, 0x2e, 0x9b, 0x90, 0xe3, 0xa0, 0x25, 0xf1,
            0x6d, 0x1a, 0x47, 0xb9, 0x8a, 0x7b, 0x64, 0x1c, 0x2f, 0xd7, 0xac, 0xcb, 0xbc, 0x82, 0xd6, 0x7e,
            0x75, 0x9a, 0x5b, 0x26, 0xd6, 0x71, 0x2c, 0x71, 0x1d, 0x44, 0x57, 0x3d, 0xd1, 0x19, 0x9e, 0x09,
            0xc7, 0x4c, 0x19, 0xd4, 0x3c, 0x7e, 0x6c, 0xb4, 0x18, 0x5b, 0xa4, 0xc8, 0x33, 0xf9, 0x32, 0x67,
            0x6c, 0x4a, 0x4d, 0x92, 0xac, 0xe5, 0x0d, 0xf3, 0x72, 0x41, 0x32, 0x19, 0xda, 0xe2, 0x86, 0x60,
            0x5b, 0xd4, 0xe2, 0x6b, 0x33, 0x60, 0xce, 0x1f, 0xfd, 0xe4, 0x52, 0x66, 0x3d, 0x30, 0xb5, 0x0a,
            0x46, 0x40, 0x69, 0x52, 0x0e, 0x12, 0x81, 0x57, 0x58, 0xb8, 0x25, 0x6c, 0x86, 0xd2, 0xb4, 0x4d,
            0xce, 0xa0, 0xa9, 0xe9, 0x1c, 0xc5, 0xd9, 0x45, 0x7d, 0xb6, 0x89, 0x12, 0xf7, 0x8b, 0x74, 0x5b,
            0x83, 0xb8, 0x12, 0xef, 0xfe, 0xc8, 0x18, 0xc1, 0x12, 0xf2, 0xd7, 0xe1, 0xb7, 0x42, 0xc7, 0xe5,
            0x1d, 0xd2, 0x7b, 0x90, 0xfd, 0x61, 0x29, 0x12, 0xf3, 0x20, 0xb8, 0x12, 0xe0, 0x64, 0x09, 0xfc,
            0xf0, 0x4d, 0xef, 0x5e, 0xde, 0x76, 0x2c, 0x93, 0x86, 0xde, 0x7f, 0xe5, 0x39, 0x80, 0x52, 0x12,
            0xf5, 0x72, 0x6a, 0xa1, 0x49, 0x73, 0x03, 0x8e, 0x01, 0xf8, 0x50, 0x77, 0x26, 0xa2, 0xfa, 0x7f,
            0xd8, 0xcf, 0x3e, 0x54, 0x3b, 0x5d, 0x8a, 0xc2, 0x6c, 0x40, 0x38, 0x70, 0x66, 0x60, 0x3b, 0x60,
            0x2e, 0xac, 0x9a, 0x3c, 0xcd, 0x41, 0x48, 0xda, 0x8d, 0x44, 0x8b, 0xae, 0xfd, 0xad, 0x73, 0x23,
            0x2d, 0xe4, 0x5b, 0xfc, 0x64, 0x88, 0x91, 0x3c, 0xa2, 0xaf, 0x02, 0x04, 0xb5, 0x12, 0x7c, 0xdf,
            0x59, 0xae, 0x7d, 0x31, 0x04, 0x2f, 0x05, 0x80, 0xcd, 0x81, 0x46, 0xd4, 0xc5, 0xba, 0x4f, 0xf1,
            0x69, 0x0f, 0x9e, 0x6c, 0x26, 0x44, 0x86, 0x6c, 0x89, 0xd7, 0xba, 0x94, 0x7e, 0x11, 0x11, 0x33,
            0x2a, 0x69, 0xa2, 0x9d, 0x2c, 0x77, 0x91, 0x8e, 0xa0, 0x89, 0x21, 0x27, 0x70, 0x1f, 0x15, 0xeb,
            0xc6, 0xb3, 0x2f, 0x45, 0xe0, 0xcc, 0x92, 0x61, 0x18, 0x1d, 0x7c, 0x41, 0x43, 0x14, 0x5e, 0xdc,
            0xdd, 0xe8, 0x5a, 0x96, 0x4e, 0x37, 0xa0, 0x83, 0xc0, 0x6f, 0xbe, 0xe1, 0xde, 0x45, 0xf4, 0xe8,
            0x0d, 0x6f, 0xb7, 0x83, 0xbe, 0xbb, 0x02, 0x0b, 0x10, 0x8f, 0x65, 0xc1, 0x65, 0x60, 0xa4, 0x69,
            0x8e, 0x47, 0xd8, 0xcc, 0xc7, 0xb5, 0x14, 0xc7, 0xbb, 0x98, 0x81, 0x2e, 0x5c, 0xfa, 0x85, 0x8d,
            0xae, 0x5f, 0x67, 0x25, 0x07, 0x75, 0x32, 0xb7, 0xab, 0xd6, 0xc2, 0x9f, 0x04, 0x1a, 0x7b, 0x11,
            0x89, 0xc0, 0xa8, 0xc2, 0x46, 0xbd, 0x12, 0x1f, 0xcb, 0xb2, 0xfa, 0xbe, 0x9f, 0x28, 0x0d, 0xa8,
            0xf7, 0x5b, 0x94, 0x99, 0x8d, 0x32, 0xbc, 0xf2, 0x4e, 0x37, 0xa7, 0xa1, 0x06, 0xaa, 0x99, 0x8f,
            0x80, 0x8b, 0x53, 0xce, 0xa3, 0x09, 0x76, 0x6e, 0x54, 0xeb, 0x08, 0xd1, 0x85, 0xbb, 0xbc, 0x58,
            0x71, 0xb6, 0x88, 0xff, 0x88, 0xa2, 0xf7, 0x0d, 0xe7, 0x30, 0xd1, 0x90, 0x04, 0x2f, 0xb7, 0x1c,
            0x3b, 0x81, 0x52, 0x89, 0xc4, 0x92, 0xc0, 0x0d, 0xb0, 0xdb, 0xab, 0x15, 0xf7, 0x16, 0xe9, 0x45,
            0x24, 0x89, 0x69, 0x11, 0xd9, 0xa8, 0xb8, 0x27, 0x30, 0x8d, 0xb8, 0xda, 0xc5, 0xda, 0x9c, 0x89,
            0xe2, 0x30, 0x62, 0x58, 0xa3, 0xef, 0x50, 0x58, 0xa4, 0xd6, 0xf6, 0x73, 0x01, 0x6f, 0x35, 0x30,
            0xa4, 0xe8, 0x76, 0xc5, 0xb3, 0x99, 0x5f, 0xb3, 0x0f, 0xeb, 0xf2, 0x9d, 0xae, 0x57, 0x25, 0xf4,
            0x8f, 0x3b, 0xac, 0xc3, 0x57, 0xf7, 0xcc, 0x45, 0xab, 0xf7, 0xc0, 0xa4, 0x2f, 0x7d, 0x7a, 0x43,
            0x0a, 0xf4, 0x97, 0x68, 0xe3, 0x53, 0x0c, 0x39, 0x45, 0x63, 0x71, 0xcc, 0x74, 0xfb, 0xf1, 0x7f,
            0xdf, 0xe4, 0xed, 0xe2, 0x61, 0xa3, 0x3e, 0xc2, 0x29, 0x01, 0x83, 0x77, 0x08, 0x0c, 0xf8, 0xe7,
            0x9b, 0x9f, 0x0e, 0xab, 0x58, 0x12, 0x37, 0x3f, 0xcd, 0xe0, 0x87, 0x33, 0xc7, 0x04, 0xdc, 0xe6,
            0xe5, 0xac, 0xcc, 0xeb, 0x08, 0xb1, 0xfb, 0x87, 0x8d, 0xdf, 0x92, 0x5b, 0x56, 0xc7, 0x13, 0x0b,
            0x5c, 0x2f, 0x9d, 0x76, 0x74, 0x41, 0x6d, 0x18, 0xd3, 0x13, 0x0f, 0x73, 0x85, 0x45, 0x03, 0x68,
            0x94, 0x85, 0x35, 0x20, 0x7c, 0x9c, 0xf8, 0x02, 0xb4, 0x5f, 0xbf, 0x46, 0xbd, 0x5c, 0x26, 0x5f,
            0x12, 0x2d, 0x6a, 0xe8, 0x53, 0x83, 0x78, 0xf0, 0xb3, 0x33, 0x1a, 0xe9, 0x64, 0x05, 0x23, 0x98,
            0x7f, 0xbc, 0x8d, 0xe5, 0xf3, 0x55, 0xc4, 0x92, 0x6b, 0xca, 0x40, 0x4b, 0x4b, 0xf5, 0x75, 0xc2,
            0x87, 0x27, 0xc8, 0x5f, 0x66, 0x0d, 0xb1, 0x37, 0x1f, 0x73, 0x57, 0xa9, 0xa5, 0x98, 0x6d, 0xb7,
            0x29, 0x7b, 0xae, 0x5a, 0x61, 0x90, 0xcf, 0xc8, 0x94, 0x2d, 0x05, 0x6f, 0x66, 0xb1, 0xdf, 0x32,
            0xe1, 0x67, 0xc9, 0xdd, 0xc8, 0xa4, 0x15, 0x4d, 0x57, 0x66, 0x17, 0xa2, 0x13, 0x7a, 0xb6, 0x65,
            0x5d, 0x0b, 0x41, 0xee, 0x30, 0x59, 0x65, 0xf9, 0xd4, 0x21, 0x9f, 0x04, 0x19, 0x27, 0xb4, 0xe2,
            0x3a, 0xfa, 0x09, 0x8a, 0x3c, 0x29, 0x99, 0x7e, 0x34, 0x28, 0xdd, 0x51, 0xeb, 0x19, 0x79, 0x02,
            0x49, 0x06, 0x02, 0xb1, 0xb8, 0x7f, 0x1e, 0xea, 0x46, 0x87, 0x90, 0xed, 0x2e, 0x5c, 0xb5, 0x5e,
            0x25, 0x9b, 0xab, 0xdc, 0x7f, 0x77, 0x6f, 0x62, 0x44, 0xfb, 0x54, 0x21, 0xb7, 0xed, 0x3f, 0x95,
            0x61, 0xec, 0x45, 0x05, 0xdf, 0x7c, 0xb4, 0x18, 0x32, 0x8c, 0xde, 0x9a, 0x42, 0x9b, 0x92, 0x9e,
            0x3f, 0x8d, 0xa2, 0xe2, 0x89, 0x7d, 0x33, 0x81, 0x83, 0x29, 0xc1, 0x14, 0x3c, 0x46, 0x0b, 0x90,
            0x3c, 0xb3, 0xf4, 0x5d, 0x39, 0xdf, 0x9a, 0xfa, 0x58, 0x9d, 0x2d, 0x04, 0x0d, 0xc2, 0xcc, 0x40,
            0xbe, 0x40, 0xa1, 0xce, 0xf5, 0x02, 0xe2, 0x6b, 0xad, 0xcc, 0xc7, 0x9f, 0x2d, 0x8b, 0x25, 0x17,
            0x00, 0x9e, 0xf6, 0xf7, 0x71, 0x5b, 0xf3, 0xfc, 0xe2, 0x39, 0x2b, 0xf0, 0x8a, 0x93, 0x8b, 0xb3,
            0x3e, 0xa0, 0x76, 0xa5, 0x88, 0x56, 0x25, 0x28, 0x06, 0x30, 0xa5, 0x5e, 0xcc, 0xb0, 0x9c, 0x20,
            0x91, 0xbf, 0xe9, 0xc1, 0xf6, 0x04, 0xfa, 0xbd, 0x49, 0xd3, 0x99, 0x47, 0x80, 0x77, 0x21, 0x2a,
            0xa1, 0xaf, 0xea, 0xee, 0xe1, 0x5f, 0x71, 0x5a, 0xab, 0x7f, 0x30, 0xa3, 0x51, 0x98, 0xfb, 0x4e,
            0xb5, 0x86, 0xf1, 0xb1, 0xdd, 0x35, 0x7b, 0xc3, 0x0f, 0xc4, 0x6e, 0x4a, 0x27, 0x0d, 0xb2, 0xbc,
            0xce, 0x75, 0xf0, 0xca, 0xd3, 0xfe, 0xb5, 0xa0, 0x36, 0x87, 0x7d, 0x47, 0x01, 0xc3, 0x2e, 0x76,
            0xda, 0xf7, 0xeb, 0xdd, 0x9d, 0x3d, 0x3e, 0x85, 0x53, 0xaf, 0x0f, 0x23, 0xda, 0x52, 0xd5, 0x49,
            0x69, 0x08, 0x10, 0x3e, 0x58, 0x64, 0x7f, 0x57, 0xa9, 0xea, 0x68, 0x5d, 0x94, 0xd5, 0x6e, 0x0e,
            0x51, 0x81, 0x92, 0x97, 0x8d, 0xb9, 0x08, 0x83, 0x85, 0x6c, 0x8f, 0x6c, 0xa0, 0x93, 0x1c, 0xfb,
            0xee, 0xda, 0x5a, 0xc7, 0xe3, 0x61, 0xa1, 0xb9, 0x5b, 0x76, 0x83, 0x45, 0xd9, 0x9a, 0xf0, 0x48,
            0x52, 0xd7, 0x7f, 0xcd, 0x8c, 0x59, 0xa7, 0x1f, 0x89, 0x2c, 0x4e, 0x23, 0x87, 0x7f, 0x22, 0x4e,
            0x1c, 0x8a, 0x54, 0x57, 0x8e, 0xb6, 0xc4, 0xf9, 0xe6, 0xae, 0xd2, 0x5e, 0xb0, 0x25, 0x9d, 0x33,
            0x6c, 0x10, 0x66, 0x2a, 0x4c, 0x84, 0x08, 0x31, 0x49, 0x98, 0x57, 0x42, 0xfd, 0x3a, 0xbe, 0x98,
            0x4a, 0x04, 0x92, 0x45, 0xba, 0x0d, 0x0a, 0xcb, 0x3c, 0xff, 0xf9, 0x0e, 0xf1, 0x0c, 0x7f, 0x01,
            0x72, 0x55, 0x1e, 0x43, 0xd0, 0x4b, 0x9b, 0x72, 0x07, 0xdc, 0xf7, 0xc0, 0x26, 0x29, 0xda, 0x00,
            0x69, 0xc8, 0x12, 0x5d, 0x2f, 0x9c, 0x95, 0x1f, 0x91, 0x48, 0x30, 0x7f, 0x65, 0x72, 0x4a, 0xf1,
            0x7d, 0x06, 0x3c, 0xa8, 0xee, 0x7f, 0xc1, 0xf5, 0x0c, 0xa2, 0xcd, 0xec, 0xd5, 0x9e, 0x9e, 0xe7,
            0xe2, 0xab, 0x1f, 0xe4, 0xdf, 0xa3, 0xb8, 0x13, 0xca, 0x43, 0x14, 0x79, 0xaa, 0x03, 0xff, 0x48,
            0x43, 0x12, 0x3e, 0xfb, 0x11, 0x76, 0xdc, 0x8f, 0xdd, 0x5f, 0x5f, 0xea, 0x70, 0xd3, 0xa1, 0xc7,
            0xbf, 0x31, 0xe4, 0x40, 0xdd, 0xc0, 0xdf, 0x94, 0xbd, 0xfb, 0xe5, 0x18, 0x41, 0x2a, 0x02, 0x83,
            0x97, 0x54, 0xc7, 0x28, 0x2b, 0xd4, 0xe2, 0xe6, 0xba, 0x21, 0xf8, 0x72, 0x5a, 0x04, 0xe6, 0x43,
            0x19, 0x10, 0xfb, 0x8a, 0x65, 0x94, 0xe6, 0x0f, 0xfa, 0x70, 0x3a, 0x46, 0x46, 0x09, 0xa6, 0x18,
            0xe4, 0xea, 0x35, 0xa2, 0x4f, 0xae, 0x8a, 0x2f, 0xf7, 0xe7, 0x3e, 0xcf, 0x3c, 0x01, 0x15, 0x70,
            0xb9, 0x2c, 0xda, 0x11, 0x0a, 0xe5, 0x1e, 0x4c, 0xde, 0x53, 0x35, 0x4c, 0xbc, 0xcf, 0x07, 0x39,
            0xbb, 0x45, 0x0b, 0xe4, 0x28, 0x0f, 0xb1, 0xaf, 0x7e, 0xed, 0xce, 0x51, 0x77, 0x38, 0x9f, 0x9e,
            0xbb, 0xde, 0x38, 0x41, 0x16, 0x29, 0x6a, 0x2c, 0x65, 0x5f, 0x51, 0xf4, 0xb6, 0x17, 0xda, 0x06,
            0x71, 0xe2, 0x07, 0xf2, 0xf0, 0xa3, 0xb6, 0x21, 0x37, 0x6b, 0x2c, 0x26, 0x68, 0x7e, 0xd7, 0x81,
            0x39, 0xd8, 0x17, 0x1a, 0x02, 0xfa, 0xf9, 0x82, 0xd0, 0x1d, 0xe1, 0x44, 0x05, 0x0b, 0xaf, 0xfa,
            0x30, 0x74, 0x1d, 0x90, 0x63, 0x2a, 0x81, 0x96, 0x48, 0x9b, 0x08, 0x0d, 0x93, 0x2e, 0x2a, 0xfe,
            0xa9, 0x04, 0x10, 0x68, 0xf1, 0x0d, 0x3d, 0x4c, 0xf2, 0xb7, 0x6c, 0xae, 0xcb, 0x2b, 0xa4, 0x89,
            0xd4, 0x28, 0x42, 0x55, 0x98, 0x69, 0xc6, 0xcc, 0x75, 0xb8, 0xa0, 0xca, 0xf7, 0x78, 0x4a, 0x50,
            0x14, 0xd8, 0x51, 0xf7, 0xd1, 0x1f, 0x53, 0xe5, 0x0f, 0x45, 0xfe, 0xaa, 0x0a, 0x11, 0xef, 0xc2,
            0x4d, 0x31, 0xa9, 0x52, 0xb8, 0x92, 0x5a, 0xab, 0xd4, 0x91, 0x4b, 0x48, 0x35, 0x34, 0x51, 0x83,
            0xba, 0x4d, 0x88, 0xa1, 0x76, 0x1b, 0xb0, 0xc3, 0xa2, 0xdf, 0x46, 0x52, 0x32, 0x32, 0x22, 0x0c,
            0xad, 0xe2, 0x31, 0x08, 0xf0, 0xe2, 0xde, 0x21, 0xcc, 0x88, 0x07, 0xde, 0x43, 0x1e, 0x8d, 0xe8,
            0x64, 0x81, 0xc2, 0x4d, 0x12, 0x60, 0xd9, 0x2d, 0x4e, 0x1f, 0x87, 0xcc, 0x0b, 0xed, 0x3d, 0xcc,
            0x3f, 0xe5, 0xfa, 0xf3, 0xa3, 0xa4, 0x0f, 0xa5, 0x13, 0xd5, 0x23, 0xbc, 0x5b, 0xfd, 0xb5, 0x4f,
            0xf1, 0xc2, 0x33, 0x5b, 0xb0, 0x23, 0xc8, 0x5c, 0x6c, 0x8a, 0xce, 0x8d, 0xd0, 0xfb, 0x3e, 0xab,
            0xaf, 0x15, 0xc9, 0x16, 0xc9, 0x66, 0x8e, 0x15, 0x23, 0x3d, 0xe0, 0x13, 0xac, 0xb7, 0x69, 0xe0,
            0xbd, 0xd9, 0x66, 0x16, 0x48, 0xf1, 0x34, 0x7e, 0xab, 0x13, 0x1b, 0xdf, 0x39, 0x50, 0xbe, 0xbd,
            0xc3, 0xf1, 0xe2, 0x1b, 0xe9, 0xb2, 0x77, 0xf4, 0x1c, 0x19, 0x7a, 0x24, 0x2e, 0xe6, 0x1b, 0xf7,
            0x3c, 0xe8, 0xa0, 0xa7, 0x6a, 0x6d, 0xfd, 0x71, 0x6e, 0xae, 0x3e, 0xa2, 0xa6, 0x16, 0xc0, 0x06,
            0x25, 0x7c, 0x3e, 0x8d, 0xb7, 0xd0, 0x6b, 0x5c, 0xbb, 0x19, 0x13, 0x59, 0x94, 0x84, 0x19, 0x0f,
            0x9f, 0xf0, 0xa7, 0x6f, 0x2d, 0x41, 0x45, 0xba, 0x3e, 0x6c, 0xae, 0x89, 0x13, 0x0c, 0xc3, 0xb2,
            0x23, 0x9c, 0xb8, 0x9e, 0x53, 0x45, 0xb0, 0xd2, 0xdf, 0x65, 0x99, 0x67, 0x5c, 0xc0, 0xbd, 0xf0,
            0x8e, 0x97, 0xd9, 0x5a, 0x8c, 0x38, 0x83, 0x1b, 0x15, 0x7f, 0xfe, 0xd7, 0xba, 0x1f, 0x73, 0xa6,
            0x4c, 0x64, 0x16, 0x95, 0xea, 0x21, 0x75, 0x1e, 0x42, 0x38, 0x9c, 0xa0, 0x24, 0x89, 0x68, 0x04,
            0x47, 0xe3, 0x77, 0xc1, 0xd9, 0x0b, 0xc3, 0xa8, 0x31, 0x9b, 0xa1, 0xd8, 0x94, 0x68, 0x48, 0x87,
            0x80, 0x34, 0xe2, 0x9f, 0x63, 0xfc, 0x93, 0xba, 0xed, 0x38, 0x94, 0xcb, 0x53, 0x04, 0xb8, 0xab,
            0x06, 0x2c, 0x3a, 0x2b, 0x15, 0xb2, 0x9c, 0x86, 0xfe, 0x2b, 0x8a, 0xed, 0xb8, 0x2d, 0xf6, 0x83,
            0x6d, 0x7c, 0xe8, 0x22, 0x52, 0xae, 0x00, 0xb0, 0xd8, 0x40, 0x2d, 0xce, 0x13, 0xad, 0x69, 0x8b,
            0x85, 0xfe, 0x1f, 0x0e, 0xe9, 0xd7, 0x27, 0xbb, 0xd8, 0xc9, 0xb4, 0x72, 0x3f, 0x08, 0x01, 0xd3,
            0x8c, 0xa7, 0x2e, 0x73, 0x09, 0xde, 0x4f, 0x8f, 0x4b, 0x39, 0x6f, 0xa4, 0xcd, 0xee, 0x6b, 0x1b,
            0x89, 0x6c, 0xaf, 0x7c, 0x61, 0x9b, 0xc6, 0x0a, 0x30, 0xba, 0x32, 0x7e, 0xe6, 0xfd, 0x72, 0x80,
            0x6a, 0xc4, 0x72, 0x17, 0x9a, 0xfc, 0xa3, 0xab, 0xcb, 0x3c, 0x73, 0x47, 0xe4, 0x94, 0x08, 0x19,
            0x2d, 0x99, 0x9b, 0x2b, 0xa8, 0x91, 0xb0, 0x84, 0x63, 0x89, 0x80, 0x59, 0xfd, 0x8f, 0xdd, 0x50,
            0x10, 0x0e, 0x89, 0xfb, 0x0e, 0x72, 0x42, 0x56, 0x86, 0xa1, 0x78, 0xcb, 0xae, 0x27, 0xc4, 0x69,
            0x7b, 0x09, 0x22, 0xfb, 0xce, 0x65, 0x5b, 0x08, 0xca, 0x22, 0x31, 0x4d, 0x73, 0xfa, 0x35, 0x87,
            0xa2, 0xfe, 0x0c, 0xd1, 0x6b, 0x51, 0x4e, 0xca, 0xa0, 0x99, 0xfe, 0xd8, 0x26, 0x70, 0xe5, 0xa6,
            0x79, 0x77, 0x47, 0xcb, 0x7a, 0x42, 0x8f, 0x55, 0x4a, 0x36, 0x25, 0xc6, 0xb5, 0x51, 0x33, 0x45,
            0x7a, 0x6d, 0xd7, 0x25, 0xd3, 0x91, 0x09, 0x46, 0x9b, 0x5e, 0x74, 0x9e, 0x3c, 0x71, 0x70, 0x87,
            0x22, 0x68, 0x6f, 0x46, 0x89, 0xdf, 0x41, 0x81, 0x07, 0x8f, 0x7c, 0xb2, 0x3f, 0x1a, 0xd5, 0x7f,
            0xb5, 0xfa, 0x76, 0x6a, 0x9e, 0x5d, 0x89, 0x9a, 0x7c, 0x20, 0xed, 0x40, 0x50, 0x54, 0x9e, 0xb8,
            0x95, 0x7e, 0x19, 0x9d, 0x3a, 0x86, 0x25, 0x88, 0x94, 0x9f, 0xed, 0x88, 0x99, 0xbb, 0x80, 0xff,
            0x83, 0xa4, 0xcb, 0x43, 0x89, 0x20, 0xf4, 0xfb, 0xe3, 0xf4, 0x3f, 0xaf, 0xc9, 0xfb, 0x54, 0x33,
            0x23, 0x55, 0x12, 0xd7, 0xae, 0xe6, 0xbd, 0x1b, 0xa7, 0x40, 0x3d, 0x46, 0xd6, 0x75, 0x3f, 0xb2,
            0x82, 0x54, 0x55, 0xbe, 0x27, 0xb5, 0xcc, 0x6d, 0x4a, 0x45, 0x10, 0xc5, 0xc4, 0x67, 0x12, 0x83,
            0xe7, 0x58, 0xd2, 0x4b, 0x9c, 0xe8, 0xa8, 0x3f, 0xb8, 0x21, 0x19, 0x40, 0x07, 0xb3, 0x9f, 0x1c,
            0xac, 0xe5, 0x8f, 0xf8, 0x37, 0xaa, 0x39, 0xef, 0xcf, 0x4d, 0xed, 0xae, 0xaf, 0x3e, 0x55, 0xb7,
            0x2c, 0x2d, 0x03, 0xe8, 0x2a, 0x65, 0x1d, 0xfd, 0x26, 0x74, 0xd3, 0x21, 0xd2, 0x0b, 0xa0, 0x55,
            0x2d, 0x6c, 0xdd, 0x43, 0x38, 0xbc, 0x9e, 0x6b, 0xb8, 0xfc, 0xc7, 0x7e, 0xc7, 0x54, 0x0d, 0xb0,
            0x9c, 0x89, 0xf3, 0xc3, 0xca, 0xb0, 0xfc, 0xb7, 0x95, 0xdc, 0x9d, 0x61, 0x19, 0x35, 0x20, 0xe1,
            0x84, 0xc8, 0x5d, 0x07, 0xe7, 0x02, 0x3b, 0x67, 0x27, 0xb7, 0x2c, 0x00, 0x68, 0x70, 0x7d, 0x5e,
            0xeb, 0x84, 0xff, 0x51, 0x59, 0xb1, 0x94, 0x1c, 0xa8, 0x86, 0x93, 0x11, 0x0b, 0xfd, 0x97, 0xca,
            0xbf, 0x45, 0x40, 0xea, 0x92, 0x23, 0xe9, 0x1e, 0x5e, 0x52, 0x23, 0xbe, 0x7d, 0x93, 0x8c, 0xab,
            0x51, 0xb7, 0x9c, 0x8c, 0x50, 0xb4, 0xe2, 0xb9, 0xf8, 0x7d, 0xed, 0x54, 0x65, 0x0c, 0x07, 0xa6,
            0x11, 0x8c, 0x4a, 0x77, 0xb2, 0x59, 0x22, 0xc4, 0x6b, 0xe4, 0x20, 0x89, 0x9e, 0xc0, 0x7c, 0x80,
            0xcc, 0x1b, 0x2b, 0x6f, 0xbf, 0x40, 0xd3, 0xb4, 0xe7, 0x7d, 0x78, 0x8d, 0x10, 0xc3, 0x16, 0xf5,
            0x44, 0xb7, 0xc4, 0xf8, 0x82, 0x97, 0xae, 0x5d, 0x9b, 0xe3, 0xc3, 0xcc, 0xb0, 0xbd, 0xb9, 0xf3,
            0x64, 0xe4, 0xaa, 0xea, 0x7c, 0x9a, 0x08, 0x84, 0xdf, 0x4c, 0x89, 0xfa, 0xc1, 0xae, 0x5a, 0x80,
            0x64, 0x75, 0x4b, 0x6d, 0xf2, 0x7b, 0xf2, 0xad, 0xb3, 0x26, 0x20, 0xac, 0x82, 0x17, 0x2d, 0xcb,
            0x5a, 0xc9, 0x19, 0x2a, 0x06, 0x8a, 0x6f, 0x68, 0x19, 0xa7, 0xef, 0xaf, 0xb0, 0x5a, 0x17, 0xe6,
            0x46, 0xa5, 0xab, 0x50, 0xfe, 0x5a, 0x7e, 0x68, 0x5c, 0x3d, 0x9a, 0x16, 0x30, 0x2e, 0x3e, 0x1f,
            0x8e, 0xa9, 0xd7, 0x8d, 0x0e, 0xcc, 0xba, 0x4b, 0x7f, 0x3a, 0xdf, 0x23, 0x97, 0x1f, 0x24, 0x9b,
            0xe6, 0x59, 0x8d, 0x43, 0x8a, 0x8c, 0x46, 0xa8, 0x99, 0xb0, 0x9f, 0x74, 0x43, 0x6e, 0x43, 0x7b,
            0x30, 0xf6, 0x2b, 0xfd, 0x5b, 0xfa, 0xfa, 0x45, 0xc1, 0xe4, 0x1c, 0xb3, 0x2b, 0x4c, 0x44, 0x26,
            0xd8, 0xde, 0x3f, 0x7c, 0x6b, 0x93, 0xb3, 0x79, 0xd7, 0xa0, 0x8d, 0xa2, 0x2f, 0x56, 0x4c, 0x9e,
            0xfe, 0xcd, 0xa2, 0x65, 0xae, 0xdd, 0xec, 0x8a, 0x83, 0x28, 0x8b, 0x69, 0xbc, 0xdc, 0xf7, 0xc7,
            0x6c, 0x45, 0x2b, 0x83, 0x38, 0x1c, 0x51, 0xf1, 0x6a, 0xa3, 0x84, 0xa9, 0x5e, 0x2c, 0x3e, 0x46,
            0xf5, 0xc8, 0x8b, 0x8b, 0xe3, 0x18, 0xb9, 0xf3, 0x80, 0xc2, 0x78, 0xa8, 0xe8, 0xc9, 0xc6, 0x82,
            0x96, 0x8f, 0xf4, 0x74, 0x42, 0xa4, 0x4d, 0x38, 0x50, 0xe4, 0x4b, 0xa8, 0x2a, 0x39, 0xbb, 0x5c,
            0x9d, 0x46, 0x4c, 0xd4, 0x09, 0xbf, 0xf2, 0x1f, 0x9b, 0x54, 0xb0, 0x90, 0xbb, 0x0f, 0x1c, 0x3b,
            0xd7, 0x78, 0x67, 0xbb, 0xdf, 0x49, 0xaf, 0x4d, 0x02, 0xe7, 0xc9, 0x57, 0x01, 0xb3, 0x27, 0x9b,
            0xd0, 0x8a, 0x58, 0x09, 0xa8, 0x7a, 0x8b, 0x33, 0x3e, 0x65, 0xdc, 0x4b, 0x42, 0xbc, 0x4f, 0xf1,
            0x78, 0x50, 0x11, 0xf0, 0x40, 0xf2, 0x12, 0x60, 0x1e, 0x79, 0x00, 0x5c, 0x19, 0x27, 0xca, 0x7d,
            0xf4, 0x50, 0x57, 0x72, 0x8b, 0xa3, 0x4c, 0xc9, 0x9d, 0x82, 0x2d, 0x12, 0xb7, 0xc0, 0x29, 0xb4,
            0xd9, 0x97, 0x8d, 0x48, 0xb6, 0x4a, 0x65, 0x53, 0x60, 0x1a, 0xe6, 0x00, 0xb8, 0x8a, 0x34, 0xa7,
            0x79, 0xfb, 0xfe, 0xc5, 0x87, 0x26, 0x0b, 0xde, 0x01, 0xe7, 0x5a, 0x97, 0x6e, 0x1e, 0x63, 0x86,
            0xce, 0x91, 0x13, 0xe8, 0xd3, 0x53, 0x4a, 0x9d, 0x86, 0x1a, 0xbc, 0xe9, 0xfc, 0x60, 0xa6, 0x29,
            0x0c, 0x08, 0x37, 0x53, 0x2e, 0x09, 0xc3, 0xcd, 0xf1, 0xa7, 0x23, 0x96, 0xba, 0xab, 0x1d, 0x62,
            0x05, 0x6a, 0x7b, 0x72, 0x38, 0x4b, 0x34, 0x0e, 0x6e, 0xe6, 0x5d, 0xbd, 0xcf, 0x38, 0x58, 0x01,
            0x7f, 0x50, 0x8f, 0x06, 0xe4, 0x69, 0x51, 0x39, 0x6a, 0xdd, 0xa6, 0xe4, 0xcc, 0x18, 0x9e, 0x84,
            0xb0, 0x80, 0x31, 0x64, 0x3f, 0xc9, 0x8e, 0x39, 0x9f, 0x73, 0x4d, 0x78, 0x88, 0x97, 0x39, 0x4e,
            0xdb, 0x96, 0xab, 0xc0, 0x06, 0x3b, 0x40, 0xb2, 0xeb, 0xfb, 0x5c, 0x07, 0xef, 0x05, 0x3e, 0x76,
            0x08, 0xe9, 0xba, 0x49, 0x1c, 0xf5, 0x4f, 0x4d, 0x13, 0x7b, 0xb2, 0x29, 0xae, 0x9f, 0x55, 0xc8,
            0xde, 0xcd, 0x21, 0x17, 0x9e, 0xeb, 0x24, 0xbb, 0xd1, 0xc5, 0x54, 0x5e, 0xe3, 0x7c, 0xb4, 0x9e,
            0x58, 0x85, 0x88, 0x34, 0x23, 0x54, 0xa4, 0xcf, 0x49, 0x84, 0x58, 0x2e, 0x5d, 0x7f, 0x9c, 0xde,
            0x71, 0xd2, 0x16, 0x1e, 0x80, 0xc0, 0x27, 0x7b, 0x2e, 0x23, 0x42, 0x03, 0xb6, 0x94, 0x82, 0x54,
            0xed, 0x62, 0xad, 0x70, 0xf0, 0xfa, 0xf1, 0xf7, 0xc7, 0xea, 0xae, 0xb5, 0x17, 0xbf, 0x31, 0x93,
            0x19, 0xc6, 0xad, 0xcd, 0xd8, 0xaf, 0x65, 0xd6, 0x81, 0xa8, 0x9b, 0x41, 0x41, 0x7f, 0xa5, 0x13,
            0x30, 0x4d, 0x9a, 0x98, 0xd2, 0x3f, 0xab, 0xd4, 0xf3, 0x93, 0xa9, 0x87, 0x53, 0x4e, 0xc6, 0xbb,
            0x2a, 0x30, 0x04, 0x68, 0xfc, 0x0f, 0x56, 0xe3, 0xe5, 0x9b, 0x04, 0x3a, 0xdb, 0x51, 0x81, 0xb5,
            0xff, 0x77, 0xfc, 0xdf, 0x18, 0x47, 0x14, 0xd5, 0x3f, 0x88, 0x6b, 0xf9, 0xc7, 0x05, 0xaa, 0xbf,
            0xc9, 0x81, 0x02, 0xae, 0x35, 0xd0, 0x13, 0xd0, 0x1c, 0xd4, 0x59, 0xd2, 0xbd, 0xd5, 0x43, 0x7e,
            0x54, 0x86, 0x8a, 0x23, 0x55, 0x7f, 0x77, 0x81, 0x19, 0x23, 0x26, 0x0d, 0x82, 0x5f, 0x6b, 0xf4,
            0x27, 0x7a, 0x2d, 0xcc, 0xe6, 0x23, 0xca, 0x57, 0x1a, 0xb1, 0x72, 0xfa, 0x34, 0x39, 0xf8, 0xfa,
            0x59, 0x8a, 0x0f, 0xcb, 0x2a, 0xdf, 0x76, 0x90, 0x61, 0xfb, 0x46, 0x43, 0xdd, 0x60, 0xf1, 0x1e,
            0x59, 0x77, 0xd3, 0x8c, 0xbd, 0x32, 0x0b, 0xd0, 0x8b, 0x34, 0x9c, 0x55, 0xae, 0xe2, 0x81, 0x87,
            0xb0, 0x26, 0x9b, 0xd6, 0x72, 0x61, 0x96, 0x32, 0xa9, 0xcf, 0x35, 0x14, 0xaf, 0xd7, 0x5d, 0x37,
            0xcf, 0xbe, 0x75, 0x8f, 0x37, 0x26, 0x71, 0xb4, 0xb0, 0xbc, 0x9b, 0x14, 0x02, 0x35, 0xc5, 0x98,
            0xa8, 0xfc, 0x61, 0x0c, 0xe3, 0x89, 0x88, 0x47, 0x7c, 0x2d, 0x66, 0x4e, 0x28, 0x92, 0xa5, 0x96,
            0xba, 0x09, 0x96, 0x9a, 0xff, 0x9c, 0x48, 0x91, 0x10, 0x79, 0x2c, 0xf0, 0x14, 0x0e, 0x3b, 0x18,
            0xe5, 0x81, 0x42, 0x61, 0xf9, 0x2e, 0xbc, 0x04, 0xff, 0xef, 0x2d, 0xfc, 0xfd, 0x24, 0xb1, 0xec,
            0xcb, 0x1a, 0x5d, 0x84, 0xce, 0x04, 0x5f, 0x4d, 0x90, 0xe2, 0xfe, 0xab, 0x44, 0xae, 0xaf, 0xc1,
            0x05, 0xfb, 0x18, 0xec, 0x3a
        }};
} // namespace cqp
#endif //HAVE_IDQ4P
