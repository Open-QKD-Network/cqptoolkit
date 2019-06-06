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
#include "Signals/FPGA/OnQber_NewValue.hpp"
#include "Signals/FPGA/OnVisibility_NewValue.hpp"
#include "Signals/OnUpdateSoftware_Progress.hpp"

#include "ZmqClassExchange.hpp"
#include "QuantumKey.hpp"

#if defined(_DEBUG)
    #include "msgpack.hpp"
#endif
//#include "Algorithms/Util/Hash.h"

namespace cqp
{

    using idq4p::classes::Command;
    using idq4p::domainModel::CommandId;
    using idq4p::domainModel::MessageDirection;
    using idq4p::classes::CommandCommunicator;
    using idq4p::utilities::MsgpackSerializer;

    Clavis3Device::Impl::Impl(const std::string& hostname, remote::Side::Type theSide) :
        side{theSide}
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
            LOGTRACE("Connecting to signal socket");
            signalSocket.connect(prefix + hostname + ":" + std::to_string(signalsPort));
            signalSocket.setsockopt(ZMQ_SUBSCRIBE, "");
            LOGTRACE("Connecting to management socket");
            mgmtSocket.connect(prefix + hostname + ":" + std::to_string(managementPort));
            LOGTRACE("Connecting to key socket");
            keySocket.connect(prefix + hostname + ":" + std::to_string(keyChannelPort));
            keySocket.setsockopt(ZMQ_SUBSCRIBE, "");
            // create a thread to read and process the signals
            signalReader = std::thread(&Impl::ReadSignalSocket, this);

            //GetProtocolVersion();
            GetSoftwareVersion(SoftwareId::CommunicatorService);
            GetSoftwareVersion(SoftwareId::BoardSupervisorService);
            GetSoftwareVersion(SoftwareId::RegulatorServiceAlice);
            GetSoftwareVersion(SoftwareId::RegulatorServiceBob);
            GetSoftwareVersion(SoftwareId::FpgaConfiguration);
            GetBoardInformation(BoardId::QkeComE);
            GetBoardInformation(BoardId::QkeHost);
            GetBoardInformation(BoardId::QkeFpga);
            GetBoardInformation(BoardId::QkeAlice);
            GetBoardInformation(BoardId::QkeBob);
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
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

    void cqp::Clavis3Device::Impl::PowerOn()
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

    idq4p::classes::GetBoardInformation cqp::Clavis3Device::Impl::GetBoardInformation(BoardId whichBoard)
    {
        using idq4p::classes::GetBoardInformation;
        //Serialize request
        GetBoardInformation requestCommand(static_cast<int32_t>(whichBoard));
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
        GetBoardInformation boardInfo;
        MsgpackSerializer::Deserialize<GetBoardInformation>(replyBuffer, boardInfo);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "' " + boardInfo.ToString() + ".");
        return boardInfo;
    }

    idq4p::classes::GetSoftwareVersion cqp::Clavis3Device::Impl::GetSoftwareVersion(SoftwareId whichSoftware)
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

    idq4p::classes::GetProtocolVersion cqp::Clavis3Device::Impl::GetProtocolVersion()
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
        return replyCommand;
    }

    void cqp::Clavis3Device::Impl::SetInitialKey(DataBlock key)
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

    bool Clavis3Device::Impl::GetRandomNumber(std::vector<uint8_t>& out)
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

        out = replyCommand.GetNumber();
        return replyCommand.GetState() == 1;
    }

    void cqp::Clavis3Device::Impl::Zeroize()
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

    void cqp::Clavis3Device::Impl::Request_UpdateSoftware(const std::string& filename, const std::string& filenameSha1)
    {
        using namespace idq4p::domainModel;
        using idq4p::classes::UpdateSoftware;
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

    void cqp::Clavis3Device::Impl::PowerOff()
    {
        const Command request(CommandId::PowerOff, MessageDirection::Request);
        Command reply(  CommandId::PowerOff, MessageDirection::Reply);
        LOGINFO("ManagementChannel: sending '" + request.ToString() + "'.");
        CommandCommunicator::RequestAndReply(mgmtSocket, request, reply);
        LOGINFO("ManagementChannel: received '" + reply.ToString() + "'.");
    }

    void Clavis3Device::Impl::SubscribeToSignals()
    {
        using namespace idq4p::domainModel;

        const std::vector<SignalId> subscribeTo
        {
            SignalId::OnSystemState_Changed,
            SignalId::OnQber_NewValue,
            SignalId::OnVisibility_NewValue
        };

        for(const auto sig : subscribeTo)
        {
            SubscribeToSignal(sig);
        }
    }

    void Clavis3Device::Impl::SubscribeToSignal(idq4p::domainModel::SignalId sig)
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

    void Clavis3Device::Impl::UnsubscribeSignal(idq4p::domainModel::SignalId sig)
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

    remote::Side::Type Clavis3Device::Impl::GetSide()
    {
        // TODO: Is there a way to get the side from the device?
        // can we call GetBoardInformation(QkeAlice) and check for errors?

        /*remote::Side::Type result = remote::Side_Type::Side_Type_Any;
        if(!boardInfo)
        {
            boardInfo->
            GetBoardInformation();
        }

        switch (static_cast<BoardID>(boardInfo->GetBoardId()))
        {
        case BoardID::QkeBob:
            result = remote::Side_Type::Side_Type_Bob;
            break;
        case BoardID::QkeAlice:
            result = remote::Side_Type::Side_Type_Alice;
            break;
        default:
            LOGERROR("Unknown board type: " + std::to_string(boardInfo->GetBoardId()));
        }*/

        return side;
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

                case SignalId::OnQber_NewValue:
                {
                    OnQber_NewValue signal;
                    MsgpackSerializer::Deserialize<OnQber_NewValue>(buffer, signal);
                    errorStats.QBER.Update(static_cast<double>(signal.GetValue()));
                    LOGINFO("New QBER value: " + signal.ToString());
                }
                break;

                case SignalId::OnVisibility_NewValue:
                {
                    OnVisibility_NewValue signal;
                    MsgpackSerializer::Deserialize<OnVisibility_NewValue>(buffer, signal);
                    alignementStats.visibility.Update(static_cast<double>(signal.GetValue()));
                    LOGINFO("New visibility value: " + signal.ToString());
                }
                break;
                case SignalId::OnUpdateSoftware_Progress:
                {
                    OnUpdateSoftware_Progress signal;
                    MsgpackSerializer::Deserialize<OnUpdateSoftware_Progress>(buffer, signal);
                    if(signal.GetProgress() == 100)
                    {
                        LOGINFO("Software update complete, please power cycle the device");
                        UnsubscribeSignal(SignalId::OnUpdateSoftware_Progress);
                    }
                    LOGINFO("New visibility value: " + signal.ToString());

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
#endif //HAVE_IDQ4P
