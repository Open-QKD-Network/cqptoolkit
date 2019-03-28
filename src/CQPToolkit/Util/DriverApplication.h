/*!
* @file
* @brief DriverApplication
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Application.h"
#include <memory>
#include "QKDInterfaces/Site.pb.h"
#include "Algorithms/Util/Strings.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include <condition_variable>

namespace grpc
{
    class ChannelCredentials;
    class ServerCredentials;
}
namespace cqp
{

    namespace remote
    {
        class ControlDetails;
    }
    class RemoteQKDDevice;

    class CQPTOOLKIT_EXPORT DriverApplication : public cqp::Application
    {
    public:
        DriverApplication();

        /**
         * @brief DisplayHelp
         * print the help page
         */
        void DisplayHelp(const cqp::CommandArgs::Option&);

        /**
         * @brief HandleVerbose
         * Parse commandline argument
         */
        void HandleVerbose(const cqp::CommandArgs::Option&);

        /**
         * @brief HandleQuiet
         * Parse commandline argument
         */
        void HandleQuiet(const cqp::CommandArgs::Option&);

        /**
         * @brief HandleQuiet
         * Parse commandline argument
         */
        virtual void HandleConfigFile(const cqp::CommandArgs::Option& option) = 0;

        int Main(const std::vector<std::string>& args) override;

        void WaitForShutdown();

        void ShutdownNow();
    protected: // members
        struct CommandlineNames
        {
            /// Site agent to register with
            static CONSTSTRING siteAgent = "site";
            static CONSTSTRING configFile = "config";
            static CONSTSTRING certFile = "cert";
            static CONSTSTRING certKeyFile = "cert-key";
            static CONSTSTRING rootCaFile = "rootca";
            static CONSTSTRING tls = "tls";
            static CONSTSTRING controlAddr = "control-addr";
            static CONSTSTRING switchName = "switch";
            static CONSTSTRING switchPort = "switch-port";
        };

        std::unique_ptr<cqp::RemoteQKDDevice> adaptor;
        /// credentials for making connections
        cqp::remote::Credentials creds;
        /// store common values.
        /// @details
        /// This needs to ether be attached to another config by calling set_allocated_... or deleted on destruction
        remote::ControlDetails* controlDetails = new remote::ControlDetails();

        std::shared_ptr<grpc::ChannelCredentials> channelCreds;
        std::shared_ptr<grpc::ServerCredentials> serverCreds;
    protected: // methods
        bool ParseConfigFile(const CommandArgs::Option& option, google::protobuf::Message& config);

        static bool WriteConfigFile(const google::protobuf::Message& config, const std::string& filename);

    private:

        std::atomic_bool shutdown {false};
        std::mutex shutdownMutex;
        std::condition_variable waitForShutdown;
    };

} // namespace cqp


