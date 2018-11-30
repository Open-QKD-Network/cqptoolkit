/*!
* @file
* @brief StatsDump
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "StatsDump.h"
#include "CQPAlgorithms/Logging/ConsoleLogger.h"
#include "CQPAlgorithms/Util/FileIO.h"

#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>

#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include "CQPToolkit/Util/URI.h"
#include <thread>
#include "CQPAlgorithms/Util/Strings.h"

using namespace cqp;

void StatsDump::OnServiceDetected(const RemoteHosts& newServices, const RemoteHosts&)
{
    for(const auto& service : newServices)
    {
        if(service.second.interfaces.contains(remote::IReporting::service_full_name()))
        {
            CollectStatsFrom(service.second.host + ":" + std::to_string(service.second.port));
        }
    }
}

struct Names
{
    static CONSTSTRING discovery = "nodiscovery";
    static CONSTSTRING connect = "connect";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
};

StatsDump::StatsDump()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    GrpcAllowMACOnlyCiphers();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&StatsDump::DisplayHelp, this, _1));

    definedArguments.AddOption(Names::discovery, "z", "Enable ZeroConf discovery");

    definedArguments.AddOption(Names::connect, "c", "Service to connect to")
    .HasArgument()
    .Callback(std::bind(&StatsDump::HandleService, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&StatsDump::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&StatsDump::HandleVerbose, this, _1));

    defaultFilter.set_listisexclude(true);
    defaultFilter.set_maxrate_ms(1000);
}

void StatsDump::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Outputs statistics from CQP services in CSV format.\nCopyright Bristol University. All rights reserved.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

int StatsDump::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        cout << "From, Path, ID, Units, Latest, Average, Total, Min, Max, Rate, Updated, Parameters" << std::endl;
        if(definedArguments.IsSet(Names::discovery))
        {
            sd.reset(new net::ServiceDiscovery());
            sd->Add(this);
        }

        definedArguments.GetProp(Names::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *creds.mutable_rootcertsfile());
        if(definedArguments.IsSet(Names::tls))
        {
            creds.set_usetls(true);
        }

        for(auto& serviceUrl : serviceUrls)
        {
            CollectStatsFrom(serviceUrl);
        }
    }

    while(!stopExecution)
    {
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }

    return exitCode;
}

void StatsDump::CollectStatsFrom(const std::string& address)
{
    URI uri(address);

    if(connections.find(uri.GetHostAndPort()) == connections.end())
    {
        LOGDEBUG("Connecting to " + uri.GetHostAndPort());
        connections[uri.GetHostAndPort()].name = uri.GetHostAndPort();
        connections[uri.GetHostAndPort()].channel = grpc::CreateChannel(uri.GetHostAndPort(), LoadChannelCredentials(creds));
        connections[uri.GetHostAndPort()].task.reset(
            new std::thread(&StatsDump::ReadStats, this, uri.GetHostAndPort(), remote::IReporting::NewStub(connections[uri.GetHostAndPort()].channel))
        );
    }
}

void StatsDump::ReadStats(std::string from, std::unique_ptr<remote::IReporting::Stub> stub)
{
    LOGDEBUG("Reader starting");
    using remote::SiteAgentReport;
    grpc::ClientContext ctx;
    remote::ReportingFilter filter = defaultFilter;
    remote::SiteAgentReport report;
    //auto& output = std::cout;
    std::stringstream output;
    auto reader = stub->GetStatistics(&ctx, filter);

    while(reader && reader->Read(&report))
    {
        output << from << ", ";

        bool addPathSep = false;

        for(const auto& pathSegment : report.path())
        {
            if(addPathSep)
            {
                output << ":";
            }
            else
            {
                addPathSep = true;
            }
            output << pathSegment;
        }

        output << ", " << report.id() << ", ";

        switch (report.unit())
        {
        case SiteAgentReport::Units::SiteAgentReport_Units_Complex:
            output << "Complex";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_Count:
            output << "Count";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_Milliseconds:
            output << "Milliseconds";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_Decibels:
            output << "Decibels";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_Hz:
            output << "Hz";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_Percentage:
            output << "Percentage";
            break;
        case SiteAgentReport::Units::SiteAgentReport_Units_SiteAgentReport_Units_INT_MAX_SENTINEL_DO_NOT_USE_:
        case SiteAgentReport::Units::SiteAgentReport_Units_SiteAgentReport_Units_INT_MIN_SENTINEL_DO_NOT_USE_:
            break;
        } // switch units

        output << ", ";

        switch(report.value_case())
        {
        case SiteAgentReport::kAsDouble:
            output
                    << report.asdouble().latest() << ", "
                    << report.asdouble().average() << ", "
                    << report.asdouble().total() << ", "
                    << report.asdouble().min() << ", "
                    << report.asdouble().max();
            break;
        case SiteAgentReport::kAsLong:
            output
                    << report.aslong().latest() << ", "
                    << report.aslong().average() << ", "
                    << report.aslong().total() << ", "
                    << report.aslong().min() << ", "
                    << report.aslong().max();
            break;
        case SiteAgentReport::kAsUnsigned:
            output
                    << report.asunsigned().latest() << ", "
                    << report.asunsigned().average() << ", "
                    << report.asunsigned().total() << ", "
                    << report.asunsigned().min() << ", "
                    << report.asunsigned().max();
            break;
        case SiteAgentReport::VALUE_NOT_SET:
            break;
        }

        output << ", " << report.rate() << ", " << report.updated().seconds() << "." << report.updated().nanos();

        for(const auto& param : report.parameters())
        {
            output << ", " << param.first + "=" + param.second;
        }

        output << std::endl;

        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(outputLock);
            std::cout << output.str();
        }/*lock scope*/
    } // while

    LogStatus(reader->Finish());
    LOGDEBUG("Reader finished");
}

CQP_MAIN(StatsDump)
