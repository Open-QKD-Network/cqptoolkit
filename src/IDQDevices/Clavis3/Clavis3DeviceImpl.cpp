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
#include "Clavis3DeviceImpl.h"
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

#include "ZmqClassExchange.hpp"
#include "QuantumKey.hpp"

//#include "Algorithms/Util/Hash.h"

namespace cqp
{

    using idq4p::classes::Command;
    using idq4p::domainModel::CommandId;
    using idq4p::domainModel::MessageDirection;
    using idq4p::classes::CommandCommunicator;
    using idq4p::utilities::MsgpackSerializer;

    Clavis3Device::Impl::Impl(const std::string& hostname)
    {
        mgmtSocket.connect(hostname + ":" + std::to_string(managementPort));
        signalSocket.connect(hostname + ":" + std::to_string(managementPort));
        keySocket.connect(hostname + ":" + std::to_string(keyChannelPort));
        keySocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        // create a thread to read and process the signals
        signalReader = std::thread(&Impl::ReadSignalSocket, this);
    }

    cqp::Clavis3Device::Impl::~Impl()
    {
        shutdown = true;

        //Serialize request
        // Send request
        const Command request(CommandId::UnsubscribeAllSignals, MessageDirection::Request);
        Command reply(  CommandId::UnsubscribeAllSignals, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
        keySocket.close();
        mgmtSocket.close();
        signalSocket.close();

        if(signalReader.joinable())
        {
            signalReader.join();
        }
    }

    void cqp::Clavis3Device::Impl::Request_PowerOn()
    {
        using namespace idq4p::classes;
        using namespace idq4p::utilities;
        using namespace idq4p::domainModel;

        const Command request(CommandId::PowerOn, MessageDirection::Request);
        Command reply(  CommandId::PowerOn, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    void cqp::Clavis3Device::Impl::Request_GetBoardInformation()
    {
        using idq4p::classes::GetBoardInformation;
        //Serialize request
        GetBoardInformation requestCommand(1);
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<GetBoardInformation>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::GetBoardInformation, MessageDirection::Request);
        Command reply(  CommandId::GetBoardInformation, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        GetBoardInformation replyCommand;
        MsgpackSerializer::Deserialize<GetBoardInformation>(replyBuffer, replyCommand);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
    }

    void cqp::Clavis3Device::Impl::Request_GetSoftwareVersion()
    {
        using idq4p::classes::GetSoftwareVersion;
        //Serialize request
        GetSoftwareVersion requestCommand(1);
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
    }

    void cqp::Clavis3Device::Impl::Request_GetProtocolVersion()
    {
        using idq4p::classes::GetProtocolVersion;
        // Send request
        const Command request(CommandId::GetProtocolVersion, MessageDirection::Request);
        Command reply(  CommandId::GetProtocolVersion, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        GetProtocolVersion replyCommand;
        MsgpackSerializer::Deserialize<GetProtocolVersion>(replyBuffer, replyCommand);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + replyCommand.ToString() + ".");
    }

    void cqp::Clavis3Device::Impl::Request_SetInitialKey(DataBlock key)
    {
        using idq4p::classes::SetInitialKey;

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

    void Clavis3Device::Impl::Request_GetRandomNumber()
    {
        using idq4p::classes::GetRandomNumber;
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
    }

    void cqp::Clavis3Device::Impl::Request_Zeroize()
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

    void cqp::Clavis3Device::Impl::Request_UpdateSoftware()
    {
        using idq4p::classes::UpdateSoftware;
        //Serialize request
        UpdateSoftware requestCommand(2, "BoardSupervisor.dfu", "0123456789ABCDEF0123456789ABCDEF01234567");
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

    void cqp::Clavis3Device::Impl::Request_PowerOff()
    {
        const Command request(CommandId::PowerOff, MessageDirection::Request);
        Command reply(  CommandId::PowerOff, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    void Clavis3Device::Impl::SubscribeToStateChange()
    {
        using idq4p::classes::SubscribeSignal;
        //Serialize request
        SubscribeSignal requestCommand(1);
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<SubscribeSignal>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::SubscribeSignal, MessageDirection::Request, requestBuffer);
        Command reply(  CommandId::SubscribeSignal, MessageDirection::Reply);

        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        // Deserialize reply
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    bool Clavis3Device::Impl::ReadKey(PSK& keyValue)
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

    void Clavis3Device::Impl::ReadSignalSocket()
    {
        using namespace idq4p::classes;
        using namespace idq4p::domainModel;

        while(!shutdown)
        {
            Signal signalWrapper;
            try
            {
                // wait for message
                SignalCommunicator::Receive(signalSocket, signalWrapper);
                // get the data from the message
                msgpack::sbuffer buffer;
                signalWrapper.GetBuffer(buffer);

                LOGINFO("Signal " + SignalId_ToString(signalWrapper.GetId()) + " received.");

                // handle signals we're interested in
                switch (signalWrapper.GetId())
                {
                case SignalId::OnSystemState_Changed:
                {
                    OnSystemState_Changed signal;
                    MsgpackSerializer::Deserialize<OnSystemState_Changed>(buffer, signal);
                    state = signal.GetState();
                    LOGINFO("State changed to: " + signal.ToString());
                }
                break;

                default:
                    // do nothing
                    break;
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
    } // ReadSignalSocket

} // namespace cqp
