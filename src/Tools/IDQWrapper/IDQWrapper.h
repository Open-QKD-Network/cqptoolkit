/*!
* @file
* @brief IDQWrapper
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPAlgorithms/Util/Application.h"
#include "CQPAlgorithms/Util/CommandArgs.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include "CQPToolkit/Drivers/Clavis.h"
#include "CQPToolkit/Drivers/IDQSequenceLauncher.h"
#include "QKDInterfaces/IIDQWrapper.grpc.pb.h"
#include <grpcpp/server.h>
#include <queue>
#include "CQPAlgorithms/Datatypes/Keys.h"

/**
 * @brief The IDQWrapper program
 * Listens for commands to start the QKDSequence program to interface with the Clavis devices from IDQuantique
 * It can be run from within a docker container to use multiple devices on the same host
 */
class IDQWrapper : public cqp::Application, public cqp::remote::IIDQWrapper::Service
{
public:
    IDQWrapper();

    /**
     * @brief DisplayHelp
     * print the help page
     */
    void DisplayHelp(const cqp::CommandArgs::Option&);

    /**
     * @brief HandleVerbose
     * Parse commandline argument
     */
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /**
     * @brief HandleQuiet
     * Parse commandline argument
     */
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

    ///@{
    /// IIDQWrapper interface

    /// @copydoc cqp::remote::IIDQWrapper::StartQKDSequence
    /// @param context
    grpc::Status StartQKDSequence(
        grpc::ServerContext* context,
        const cqp::remote::IDQStartOptions* request,
        grpc::ServerWriter<cqp::remote::SharedKey>* writer) override;

    /// @copydoc cqp::remote::IIDQWrapper::UseKeyID
    /// @param context
    grpc::Status UseKeyID(
        grpc::ServerContext* context,
        grpc::ServerReader<cqp::remote::KeyIdValue>* reader,
        google::protobuf::Empty*) override;

    /// @copydoc cqp::remote::IIDQWrapper::GetDetails
    /// @param context
    grpc::Status GetDetails(
        grpc::ServerContext* context,
        const google::protobuf::Empty*,
        cqp::remote::WrapperDetails* response) override;
    ///@}
protected:
    /**
     * @brief Main
     * @param args
     * @return program exit code
     */
    int Main(const std::vector<std::string>& args) override;

    /// Credentials to use for connections
    cqp::remote::Credentials creds;
    /// server for client to connect to
    std::unique_ptr<grpc::Server> server;
    /// The device we're controlling
    std::unique_ptr<cqp::Clavis> device;
    /// access control to wait for device
    std::mutex deviceReadyMutex;
    /// to wait for device
    std::condition_variable deviceReadyCv;
    /// key id's sent from alice for us to pull from bob
    std::queue<cqp::KeyID> waitingKeyIds;
    /// access control for waitingKeyIds
    std::mutex waitingKeyIdsMutex;
    /// for waiting for changes to waitingKeyIds
    std::condition_variable waitingKeyIdsCv;

    /// The port number for our server
    int portNumber = 7000;

    /// program exit codes
    enum ExitCodes { Ok = 0, ConfigNotFound = 10, InvalidConfig = 11, UnknownError = 99 };

    /// prevent more than one client using the device at any one time
    std::mutex oneAtATime;
    /// for stopping the key reading thread
    bool keepReadingKey = false;
    /// The type of device detected
    cqp::IDQSequenceLauncher::DeviceType side = cqp::IDQSequenceLauncher::DeviceType::None;

};
