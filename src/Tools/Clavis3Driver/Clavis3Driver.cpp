/*!
* @file
* @brief Clavis3Driver
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include <iostream>
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/Application.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Util/Strings.h"
#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>
#include <google/protobuf/util/json_util.h>

#include <thread>

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

using namespace cqp;


/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class Clavis3Driver : public Application
{

    struct Names
    {
        static CONSTSTRING configFile = "config-file";
        static CONSTSTRING id = "id";
        static CONSTSTRING port = "port";
        static CONSTSTRING certFile = "cert";
        static CONSTSTRING keyFile = "key";
        static CONSTSTRING rootCaFile = "rootca";
        static CONSTSTRING tls = "tls";
        static CONSTSTRING connect = "connect";
    };

public:
    /// Constructor
    Clavis3Driver();

    /// Destructor
    ~Clavis3Driver() override = default;

    /// The application's main logic.
    /// Returns an exit code which should be one of the values
    /// from the ExitCode enumeration.
    /// @param[in] args Unprocessed command line arguments
    /// @return success state of the application
    int Main(const std::vector<std::string>& definedArguments) override;

protected:
    /// Called when the option with the given name is encountered
    /// during command line arguments processing.
    ///
    /// The default implementation does option validation, bindings
    /// and callback handling.
    /// @param[in] name Option name
    /// @param[in] value Option Value
    void HandleOption(const CommandArgs::Option& option);

    /// print the help page
    void DisplayHelp(const CommandArgs::Option& option);

    /**
     * @brief HandleVerbose
     * Make the program more verbose
     */
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * Make the program quieter
     */
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

private:
    std::shared_ptr<grpc::ChannelCredentials> clientCreds = grpc::InsecureChannelCredentials();

    /// should the help message be displayed
    bool _helpRequested = false;
    /// exit codes for this program
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, ServiceCreationFailed = 20, UnknownError = 99 };
};

Clavis3Driver::Clavis3Driver()
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    definedArguments.AddOption(Names::configFile, "c", "load configuration data from a file")
    .Bind();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&Clavis3Driver::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::id, "i", "Site Agent ID")
    .Bind();

    definedArguments.AddOption(Names::port, "p", "Listen on this port")
    .Bind();

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&Clavis3Driver::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::connect, "r", "Connect to other site")
    .Bind();

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&Clavis3Driver::HandleVerbose, this, _1));


    // Site agent drivers
    //DummyQKD::RegisterWithFactory();
    // ^^^^^^ Add new drivers here ^^^^^^^^^

}

void Clavis3Driver::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;
    string driverNames;

    definedArguments.PrintHelp(std::cout, "Creates CQP Site Agents for managing QKD systems.\nCopyright Bristol University. All rights reserved.",
                               "Drivers: " + driverNames);
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

int Clavis3Driver::Main(const std::vector<std::string> &args)
{
    Application::Main(args);

    if(!stopExecution)
    {

        using namespace std;
        using namespace zmq;
        using namespace idq4p::utilities;
        using namespace idq4p::domainModel;
        using namespace idq4p::classes;
        zmq::context_t context (1);
        zmq::socket_t _qkeMgmtSocket (context, ZMQ_REQ);

        //Serialize request
        GetBoardInformation requestCommand(1);
        msgpack::sbuffer requestBuffer;
        MsgpackSerializer::Serialize<GetBoardInformation>(requestCommand, requestBuffer);
        // Send request
        const Command request(CommandId::GetBoardInformation, MessageDirection::Request);
            Command reply(  CommandId::GetBoardInformation, MessageDirection::Reply);
        //_ui->WriteLine(UISS << "ManagementChannel: sending '" << request.ToString() << "'.");
        CommandCommunicator::RequestAndReply(_qkeMgmtSocket, request, reply);
        //Deserialize reply
        msgpack::sbuffer replyBuffer;
        reply.GetBuffer(replyBuffer);
        GetBoardInformation replyCommand;
        MsgpackSerializer::Deserialize<GetBoardInformation>(replyBuffer, replyCommand);
        //_ui->WriteLine(UISS << "ManagementChannel: received '" << reply.ToString() << "' " << replyCommand.ToString() << ".");
        /*
        using idq4p::classes::CommandCommunicator;
        using idq4p::classes::Command;

        //  Prepare our context and socket
        zmq::context_t context (1);
        zmq::socket_t socket (context, ZMQ_REQ);

        std::cout << "Connecting to hello world server..." << std::endl;
        socket.connect ("tcp://localhost:5555");

        //  Do 10 requests, waiting each time for a response
        for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
            zmq::message_t request (5);
            Command cmd;
            CommandCommunicator::Send(socket, cmd);
            memcpy (request.data (), "Hello", 5);
            std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
            socket.send (request);

            //  Get the reply.
            zmq::message_t reply;
            socket.recv (&reply);
            std::cout << "Received World " << request_nbr << std::endl;
        }*/
    }

    return 0;
}

CQP_MAIN(Clavis3Driver)
