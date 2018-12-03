/*!
* @file
* @brief IDQKeyExtraction
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/FileIO.h"

#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>

#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Datatypes/UUID.h"
#include <thread>
#include "StatsGenerator.h"

#include "CQPToolkit/Alignment/Stats.h"
#include "CQPToolkit/KeyGen/Stats.h"
#include "CQPToolkit/Session/Stats.h"
#include <algorithm>
#include "Algorithms/Util/Strings.h"

using namespace cqp;

struct Names
{
    static CONSTSTRING discovery = "nodiscovery";
    static CONSTSTRING port = "port";
    static CONSTSTRING count = "count";
    static CONSTSTRING types = "types";
    static CONSTSTRING local = "local";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
};

struct TypeNames
{
    static NAMEDSTRING(all);
    static NAMEDSTRING(alignment);
    static NAMEDSTRING(keygen);
    static NAMEDSTRING(session);
};

// storage for strings
CONSTSTRING TypeNames::alignment;
CONSTSTRING TypeNames::all;
CONSTSTRING TypeNames::keygen;
CONSTSTRING TypeNames::session;

StatsGenerator::StatsGenerator() :
    allMessageTypes
{
    {TypeNames::all, true}, {TypeNames::alignment, false}, {TypeNames::keygen, false}, {TypeNames::session, false}
},
smallIntDistribution(0, 3000),
                     percentDistribution(0.0, 1.0),
                     tinyIntDistribution(0, 1024),
                     updateDistribution(0, 1000)
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Info);

    GrpcAllowMACOnlyCiphers();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&StatsGenerator::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::discovery, "z", "Enable ZeroConf registration");

    definedArguments.AddOption(Names::port, "p", "Port number to listen on. Default = random")
    .Bind();

    definedArguments.AddOption(Names::count, "c", "The number of broadcasts to send before quiting. Default = Infinite")
    .Bind();

    std::string messageTypeNames;
    for(const auto& typeName : allMessageTypes)
    {
        if(!messageTypeNames.empty())
        {
            messageTypeNames += ", ";
        }
        messageTypeNames += typeName.first;
    }

    definedArguments.AddOption(Names::types, "t", "The type of message to send, repeat for multiple types. Default = All.\n   Possible types: " + messageTypeNames)
    .HasArgument()
    .Callback(std::bind(&StatsGenerator::HandleType, this, _1));

    definedArguments.AddOption(Names::local, "l", "Output the values being generated to stdout");

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&StatsGenerator::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&StatsGenerator::HandleVerbose, this, _1));

}

void StatsGenerator::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Outputs statistics from CQP services in CSV format.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void StatsGenerator::HandleType(const CommandArgs::Option& option)
{
    using namespace std;
    std::string valueLowered = option.value;
    std::transform(valueLowered.begin(), valueLowered.end(), valueLowered.begin(), ::tolower);
    auto foundType = allMessageTypes.find(valueLowered);

    if(foundType != allMessageTypes.end())
    {
        allMessageTypes[TypeNames::all] = false;
        foundType->second = true;
    }
    else
    {
        LOGERROR("Unknown message type: " + option.value);
        stopExecution = true;
    }
}

int StatsGenerator::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {

        definedArguments.GetProp(Names::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(Names::tls))
        {
            creds.set_usetls(true);
        }

        // server setup

        std::string myAddress = "0.0.0.0";
        int listenPort = 0;
        definedArguments.GetProp(Names::port, listenPort);

        // create our own server which all the steps will use
        grpc::ServerBuilder builder;
        // grpc will create worker threads as it needs, idle work threads
        // will be stopped if there are more than this number running
        // setting this too low causes large number of thread creation+deletions, default = 2
        builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 50);
        builder.AddListeningPort(myAddress + ":" + std::to_string(listenPort), LoadServerCredentials(creds), &listenPort);

        builder.RegisterService(&reportServer);
        // ^^^ Add new services here ^^^ //

        // start the server
        server = builder.BuildAndStart();
        if(server)
        {
            LOGINFO("Listening on " + myAddress + ":" + std::to_string(listenPort));

            if(definedArguments.IsSet(Names::discovery))
            {
                sd.reset(new net::ServiceDiscovery());

                RemoteHost sdhost;
                sdhost.name = "StatsGenerator-" + std::to_string(listenPort);
                sdhost.port = listenPort;
                sdhost.id = UUID();
                sdhost.interfaces.insert(remote::IReporting::service_full_name());
                // ^^^ Add new services here ^^^ //

                sd->SetServices(sdhost);
            }

        } // if(server)
        else
        {
            LOGERROR("Failed to create server");
            stopExecution = true;
        } // else
    } // if !stopExecution

    if(!stopExecution)
    {
        // stats gen
        size_t numBroadcasts = 0;
        definedArguments.GetProp(Names::count, numBroadcasts);
        size_t numSent = 0;

        keygen::Statistics keygenStats1;
        keygen::Statistics keygenStats2;
        align::Statistics alignmentStats;
        session::Statistics sessionStats1;
        session::Statistics sessionStats2;

        if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::keygen])
        {
            keygenStats1.SetEndpoints("SiteA.cqp:7000", "SiteB.cqp:7101");
            keygenStats2.SetEndpoints("SiteA.cqp:7000", "SiteC.cqp:7000");
            keygenStats1.Add(&reportServer);
            keygenStats2.Add(&reportServer);
            keygenStats1.Add(&statsLogger);
            keygenStats2.Add(&statsLogger);
        }
        if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::alignment])
        {
            alignmentStats.SetEndpoints("SiteA.cqp:7000", "SiteB.cqp:7101");
            alignmentStats.Add(&reportServer);
            alignmentStats.Add(&statsLogger);
        }
        if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::session])
        {
            sessionStats1.SetEndpoints("SiteA.cqp:7000", "SiteB.cqp:7101");
            sessionStats2.SetEndpoints("SiteA.cqp:7000", "SiteC.cqp:7000");
            sessionStats1.Add(&reportServer);
            sessionStats2.Add(&reportServer);
            sessionStats1.Add(&statsLogger);
            sessionStats2.Add(&statsLogger);
        }

        if(definedArguments.IsSet(Names::local))
        {
            statsLogger.SetOutput(stats::StatisticsLogger::Destination::StdOut);
        }

        LOGINFO("Starting stat generation.");
        const auto sessionStart = chrono::high_resolution_clock::now();

        // loop to complete all broadcasts
        while(!stopExecution && (numBroadcasts == 0 || numSent < numBroadcasts))
        {
            numSent++;

            LOGINFO("Broadcast number: " + std::to_string(numSent));

            if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::keygen])
            {
                LOGDEBUG("Generating Keygen statistics - Session 1");
                this_thread::sleep_for(chrono::milliseconds(updateDistribution(generator)));

                auto keyAvailable = smallIntDistribution(generator);
                keygenStats1.keyGenerated.Update(keyAvailable);
                keygenStats1.unusedKeysAvailable.Update(keyAvailable + smallIntDistribution(generator));
                keygenStats1.reservedKeys.Update(smallIntDistribution(generator));
                keygenStats1.keyUsed.Update(tinyIntDistribution(generator));

                this_thread::sleep_for(chrono::milliseconds(updateDistribution(generator)));

                LOGDEBUG("Generating Keygen statistics - Session 2");
                keyAvailable = smallIntDistribution(generator);
                keygenStats2.keyGenerated.Update(keyAvailable);
                keygenStats2.unusedKeysAvailable.Update(keyAvailable + smallIntDistribution(generator));
                keygenStats2.reservedKeys.Update(smallIntDistribution(generator));
                keygenStats2.keyUsed.Update(tinyIntDistribution(generator));
            }

            if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::alignment])
            {
                LOGDEBUG("Generating Alignment statistics");
                this_thread::sleep_for(chrono::milliseconds(updateDistribution(generator)));

                alignmentStats.overhead.Update(percentDistribution(generator));
                alignmentStats.qubitsProcessed.Update(smallIntDistribution(generator));
                alignmentStats.timeTaken.Update(chrono::milliseconds(updateDistribution(generator)));
            }

            if(allMessageTypes[TypeNames::all] || allMessageTypes[TypeNames::session])
            {
                LOGDEBUG("Generating session statistics - Session 1");
                this_thread::sleep_for(chrono::milliseconds(updateDistribution(generator)));
                sessionStats1.TimeOpen.Update(chrono::high_resolution_clock::now() - sessionStart);

                LOGDEBUG("Generating session statistics - Session 2");
                this_thread::sleep_for(chrono::milliseconds(updateDistribution(generator)));
                sessionStats2.TimeOpen.Update(chrono::high_resolution_clock::now() - sessionStart);
            }
        } // while keep going
    }

    return exitCode;
}

CQP_MAIN(StatsGenerator)
