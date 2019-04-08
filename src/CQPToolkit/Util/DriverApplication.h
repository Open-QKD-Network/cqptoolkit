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
#include "QKDInterfaces/Site.pb.h"
#include "Algorithms/Util/Strings.h"
#include "CQPToolkit/cqptoolkit_export.h"

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

    /**
     * @brief The DriverApplication class specialises the Application class for drivers
     */
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
         * @param option contains the filename
         */
        virtual void HandleConfigFile(const cqp::CommandArgs::Option& option) = 0;

        /**
         * @brief Main
         * @param args
         * @return exit code
         */
        int Main(const std::vector<std::string>& args) override;

        /**
         * @brief ShutdownNow stops all processing
         */
        void ShutdownNow() override;
    protected: // members
        /// names for long command line arguments
        struct CommandlineNames
        {
            /// Site agent to register with
            static CONSTSTRING siteAgent = "site";
            /// the config file to load
            static CONSTSTRING configFile = "config";
            /// tls public certificate to load
            static CONSTSTRING certFile = "cert";
            /// tls private key to load
            static CONSTSTRING certKeyFile = "cert-key";
            /// tls root ca
            static CONSTSTRING rootCaFile = "rootca";
            /// use tls switch
            static CONSTSTRING tls = "tls";
            /// the host:port for the control address
            static CONSTSTRING controlAddr = "control-addr";
            /// identifier for the connected switch
            static CONSTSTRING switchName = "switch";
            /// identifier for the port on the switch
            static CONSTSTRING switchPort = "switch-port";
        };

        /// bridge between the cqp::remote::IDevice interface and the driver
        std::unique_ptr<cqp::RemoteQKDDevice> adaptor;
        /// credentials for making connections
        cqp::remote::Credentials creds;
        /// store common values.
        /// @details
        /// This needs to ether be attached to another config by calling set_allocated_... or deleted on destruction
        remote::ControlDetails* controlDetails = new remote::ControlDetails();

        /// client credentials
        std::shared_ptr<grpc::ChannelCredentials> channelCreds;
        /// server credentials
        std::shared_ptr<grpc::ServerCredentials> serverCreds;
    protected: // methods
        /**
         * @brief ParseConfigFile
         * @param option
         * @param config
         * @return true on success
         */
        bool ParseConfigFile(const CommandArgs::Option& option, google::protobuf::Message& config);

        /**
         * @brief WriteConfigFile
         * @param config
         * @param filename
         * @return true on success
         */
        static bool WriteConfigFile(const google::protobuf::Message& config, const std::string& filename);

    };

} // namespace cqp


