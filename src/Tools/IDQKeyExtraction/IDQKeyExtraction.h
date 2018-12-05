/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Util/Application.h"
#include "Algorithms/Util/CommandArgs.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Util/KeyPrinter.h"
#include <mutex>
#include <condition_variable>
#include "QKDInterfaces/Site.pb.h"
#include "Algorithms/Util/Strings.h"

/// This application provides both Alice and Bob applications for driving the IDQuantique Clavis 2 QKD devices
/// via the IDQWrapper interface. Once established, the devices will be
/// instructed to begin exchanging key which will be collected and provided on the IKey interface.
class IDQKeyExtraction : public cqp::Application
{
public:
    /// Default constructor
    IDQKeyExtraction();
protected:

    /// main body of the application
    /// @param args command line arguments
    /// @returns 0 for success or error code
    virtual int Main(const std::vector<std::string> &args) override;

    /// Called when the help option is specified, it display the help
    /// @param option details of the option being parsed
    void HandleHelp(const cqp::CommandArgs::Option& option);


    /// Called when the verbose option is specified, increases the output level
    void HandleVerbose(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().IncOutputLevel();
    }

    /// Called when the quiet option is specified, decreases the output level
    void HandleQuiet(const cqp::CommandArgs::Option&)
    {
        cqp::DefaultLogger().DecOutputLevel();
    }

    /// Called when the ```device``` command line option is specified
    /// @param option details of the option being parsed
    void HandleDevice(const cqp::CommandArgs::Option& option)
    {
        devices.push_back(option.value);
    }

    /// Holds all the devices which are attached.
    std::vector<std::string> devices;

    /// mutex for the timeToExit value
    std::mutex              exitMutex;
    /// Allows the timeToExit variable to be monitored
    std::condition_variable waitForExit;
    /// Output the key to the user
    cqp::KeyPrinter kp;

    /// Names for for comandline arguments
    struct Names
    {
        /// command line option long name
        static CONSTSTRING local = "local";
        /// command line option long name
        static CONSTSTRING remote = "remote";
        /// command line option long name
        static CONSTSTRING lineAtten= "line-atten";
        /// command line option long name
        static CONSTSTRING internalPort= "internal-port";
        /// command line option long name
        static CONSTSTRING initPsk= "init-psk";


        /// command line option long name
        static CONSTSTRING CertFile = "cert";
        /// command line option long name
        static CONSTSTRING KeyFile = "key";
        /// command line option long name
        static CONSTSTRING RootCaFile = "rootca";
        /// command line option long name
        static CONSTSTRING Tls = "tls";
    };

    /// credentials to use when connecting
    cqp::remote::Credentials creds;
};
