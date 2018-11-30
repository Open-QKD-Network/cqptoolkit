/*!
* @file
* @brief CQP Toolkit -  QKDExampleUI
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16 Aug 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include <iostream>

#include "CQPToolkit/Util/KeyVerifier.h"
#include "CQPToolkit/Util/KeyPrinter.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include "CQPAlgorithms/Util/Application.h"

#include <grpc/grpc.h>
#include <grpc++/server_builder.h>
#include <grpc++/server.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/security/credentials.h>

using namespace cqp;

/**
 * @brief The ExampleConsoleApp class
 * Simple GUI for driving the QKD software
 */
class RemotingExample : public Application, public virtual IKeyCallback
{

public:
    /// Constructor
    RemotingExample();

    ///@{
    /// @name IKeyCallback interface
    void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
    ///@}


    /// The application's main logic.
    ///
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
    void handleOption(const CommandArgs::Option& option);

    /// print the help page
    void displayHelp(const CommandArgs::Option& option);

    void StartAlice(uint16_t port);

    void StartBob(const std::string& address);
private:
    /// verify the keys
    KeyVerifier keyVerifier;

    /// condition to wait for
    unsigned long keyReceived;
    std::condition_variable keyCv;
    std::mutex keyMut;

    std::string connectionAddress = "127.0.0.1:8000";
    uint16_t listenPort = 8000;
    /// should the help message be displayed
    bool _helpRequested = false;

    bool runAsAlice = true;
    bool runAsBob = true;
};

RemotingExample::RemotingExample()
{
    using namespace cqp;
    using std::placeholders::_1;
    ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Trace);

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&RemotingExample::displayHelp, this, _1));
    definedArguments.AddOption("alice", "a", "Run alice, start a service to connect to")
    .Callback(std::bind(&RemotingExample::handleOption, this, _1));
    definedArguments.AddOption("port", "p", "Alice listens on this port")
    .Callback(std::bind(&RemotingExample::handleOption, this, _1));
    definedArguments.AddOption("bob", "b", "Run bob, connect to alice")
    .Callback(std::bind(&RemotingExample::handleOption, this, _1));
    definedArguments.AddOption("addr", "r", "Connect to alice at remote address")
    .Callback(std::bind(&RemotingExample::handleOption, this, _1));
}

void RemotingExample::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
{
    keyReceived++;
    keyCv.notify_one();
}

void RemotingExample::handleOption(const CommandArgs::Option& option)
{
    if (option.longName == "help")
    {
        _helpRequested = true;
    }
    else if(option.longName == "alice")
    {
        runAsAlice = true;
        runAsBob = false;
    }
    else if(option.longName == "bob")
    {
        runAsAlice = false;
        runAsBob = true;
    }
    else if(option.longName == "port")
    {
        listenPort = static_cast<uint16_t>(std::stoul(option.value));
    }
    else if(option.longName == "addr")
    {
        connectionAddress = option.value;
    }
}

void RemotingExample::displayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "A sample HTTP server supporting the WebSocket protocol.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void RemotingExample::StartAlice(uint16_t)
{
    /// for sending alice's key to the console
    KeyPrinter keyPrinter;
    keyPrinter.SetOutputPrefix("Alice: ");

}

void RemotingExample::StartBob(const std::string&)
{
    /// for sending bob's key to the console
    KeyPrinter keyPrinter;
    keyPrinter.SetOutputPrefix("Bob: ");

    std::shared_ptr<grpc::Channel> clientChannel = grpc::CreateChannel(connectionAddress,
            grpc::InsecureChannelCredentials());

}

int RemotingExample::Main(const std::vector<std::string> &args)
{
    Application::Main(args);

    try
    {
        LOGINFO("Basic application to show the possible implementation of QKD software");

        grpc::ServerBuilder builder;
        int listenPort = 0;
        builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &listenPort);
        //builder.RegisterService(&receiver);

        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());


        if(runAsAlice)
        {
            StartAlice(listenPort);
        }

        if(runAsBob)
        {
            StartBob(connectionAddress);
        }

        std::unique_lock<std::mutex> lock(keyMut);
        keyCv.wait_for(lock, std::chrono::seconds(3), [&](){
            return keyReceived != 0;
        });
    }
    catch(const std::exception& e)
    {
        LOGERROR("Exception: " + std::string(e.what()));
    }
    return 0;
}

CQP_MAIN(RemotingExample)
