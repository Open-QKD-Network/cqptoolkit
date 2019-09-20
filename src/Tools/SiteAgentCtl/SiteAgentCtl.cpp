/*!
* @file
* @brief SiteAgentCtl
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteAgentCtl.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Auth/AuthUtil.h"
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <google/protobuf/util/json_util.h>
#include <thread>
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/URI.h"
#include "KeyManagement/KeyStores/BackingStoreFactory.h"
#include "KeyManagement/KeyStores/Utils.h"

using namespace cqp;

struct Names
{
    static CONSTSTRING start = "start";
    static CONSTSTRING stop = "stop";
    static CONSTSTRING details = "details";
    static CONSTSTRING connect = "connect";
    static CONSTSTRING listSites = "list_sites";
    static CONSTSTRING getkey = "getkey";
    static CONSTSTRING hopUrl = "hop_url";
    static CONSTSTRING hopDevice = "hop_device";
    static CONSTSTRING certFile = "cert";
    static CONSTSTRING keyFile = "key";
    static CONSTSTRING rootCaFile = "rootca";
    static CONSTSTRING tls = "tls";
    static CONSTSTRING generate = "gen-keys";
    static CONSTSTRING backingStore = "backing-store";
    static CONSTSTRING siteId = "site-id";
};

SiteAgentCtl::SiteAgentCtl()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    definedArguments.AddOption(Names::start, "b", "Tell the node to start, using the JSON values provided")
    .HasArgument()
    .Callback(std::bind(&SiteAgentCtl::HandleStart, this, _1));

    definedArguments.AddOption(Names::stop, "e", "Tell the node to stop, using the JSON values provided")
    .HasArgument()
    .Callback(std::bind(&SiteAgentCtl::HandleStop, this, _1));

    definedArguments.AddOption(Names::details, "d", "Tell the node to report it's settings")
    .Callback(std::bind(&SiteAgentCtl::HandleDetails, this, _1));

    definedArguments.AddOption(Names::listSites, "l", "List the connected sites")
    .Callback(std::bind(&SiteAgentCtl::HandleListSites, this, _1));

    definedArguments.AddOption(Names::getkey, "k", "Get a Key from the site to the destination which can be ether a host:port or a pkcs11 string")
    .HasArgument()
    .Callback(std::bind(&SiteAgentCtl::HandleGetKey, this, _1));

    definedArguments.AddOption(Names::connect, "c", "Site to connect to")
    .Bind();

    definedArguments.AddOption(Names::certFile, "", "Certificate file")
    .Bind();
    definedArguments.AddOption(Names::keyFile, "", "Certificate key file")
    .Bind();
    definedArguments.AddOption(Names::rootCaFile, "", "Certificate authority file")
    .Bind();

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&SiteAgentCtl::DisplayHelp, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&SiteAgentCtl::HandleQuiet, this, _1));

    definedArguments.AddOption(Names::tls, "s", "Use secure connections");

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&SiteAgentCtl::HandleVerbose, this, _1));

    definedArguments.AddOption(Names::generate, "g", "Generate number of keys into keystores, specifiy by repeating -x & -i")
    .Bind();
    definedArguments.AddOption(Names::backingStore, "x", "Backing store to connect to.")
    .HasArgument()
    .Callback(std::bind(&SiteAgentCtl::HandleBackingStore, this, _1));
    definedArguments.AddOption(Names::siteId, "i", "The site name for the backing store")
    .HasArgument()
    .Callback(std::bind(&SiteAgentCtl::HandleSiteId, this, _1));

}

void SiteAgentCtl::DisplayHelp(const CommandArgs::Option&)
{
    using namespace std;
    remote::PhysicalPath example;
    auto newHop = example.add_hops();
    std::string exampleJson;
    google::protobuf::util::JsonOptions jsonOptions;

    newHop->mutable_first()->set_site("siteA:1234");
    newHop->mutable_first()->set_deviceid("dummyqkd:///?side=alice&port=dummy1a");
    newHop->mutable_second()->set_site("siteB:1235");
    newHop->mutable_second()->set_deviceid("dummyqkd:///?side=bob&port=dummy1b");

    jsonOptions.add_whitespace = false;
    jsonOptions.preserve_proto_field_names = true;
    LogStatus(google::protobuf::util::MessageToJsonString(example, &exampleJson, jsonOptions));

    definedArguments.PrintHelp(std::cout, "Send commands to a running site agent.\nCopyright Bristol University. All rights reserved.", "Example JSON strings:\n" + exampleJson);
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

void SiteAgentCtl::HandleStart(const CommandArgs::Option& option)
{
    google::protobuf::util::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = false;

    Command cmd(Command::Cmd::Start);
    using namespace google::protobuf::util;

    if(LogStatus(JsonStringToMessage(option.value, &cmd.physicalPath, parseOptions)).ok())
    {
        commands.push_back(cmd);
    }
    else
    {
        definedArguments.StopOptionsProcessing();
        stopExecution = true;
    }
}

void SiteAgentCtl::HandleStop(const CommandArgs::Option& option)
{
    google::protobuf::util::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = false;

    Command cmd(Command::Cmd::Stop);
    using namespace google::protobuf::util;

    if(LogStatus(JsonStringToMessage(option.value, &cmd.physicalPath, parseOptions)).ok())
    {
        commands.push_back(cmd);
    }
    else
    {
        definedArguments.StopOptionsProcessing();
        stopExecution = true;
    }
}

void SiteAgentCtl::HandleDetails(const CommandArgs::Option&)
{
    commands.push_back(Command::Cmd::Details);
}

void SiteAgentCtl::HandleListSites(const CommandArgs::Option&)
{
    commands.push_back(Command::Cmd::List);
}

void SiteAgentCtl::HandleGetKey(const CommandArgs::Option& option)
{
    Command cmd(Command::Cmd::Key);
    cmd.destination = option.value;
    commands.push_back(cmd);
}

void SiteAgentCtl::HandleBackingStore(const CommandArgs::Option& option)
{
    backingStores.push_back(option.value);
}

void SiteAgentCtl::HandleSiteId(const CommandArgs::Option& option)
{
    siteIds.push_back(option.value);
}

void SiteAgentCtl::ListSites(remote::IKey::Stub* siteA)
{
    using namespace std;
    using namespace grpc;
    using google::protobuf::Empty;

    ClientContext ctx;
    Empty request;
    remote::SiteList response;

    LOGDEBUG("Listing key store destinations...");
    if(LogStatus(siteA->GetKeyStores(&ctx, request, &response)).ok())
    {
        for(const auto& url : response.urls())
        {
            cout << url << endl;
        }
    }
}

void SiteAgentCtl::StartNode(remote::ISiteAgent::Stub* siteA, const remote::PhysicalPath& path)
{

    using namespace grpc;
    using google::protobuf::Empty;
    ClientContext ctx;
    Empty response;

    LOGDEBUG("Starting node...");
    LogStatus(siteA->StartNode(&ctx, path, &response));
}

void SiteAgentCtl::StopNode(remote::ISiteAgent::Stub* siteA, const remote::PhysicalPath& path)
{

    using namespace grpc;
    using google::protobuf::Empty;
    ClientContext ctx;
    Empty response;

    LOGDEBUG("Stopping node...");
    LogStatus(siteA->EndKeyExchange(&ctx, path, &response));
}

void SiteAgentCtl::GetDetails(remote::ISiteAgent::Stub* siteA)
{
    using namespace std;
    using namespace grpc;
    using google::protobuf::Empty;

    ClientContext ctx;
    Empty request;
    remote::Site response;

    LOGDEBUG("Getting Site details...");
    if(LogStatus(siteA->GetSiteDetails(&ctx, request, &response)).ok())
    {
        string details;
        google::protobuf::util::JsonOptions opts;
        opts.add_whitespace = true;
        opts.preserve_proto_field_names = true;

        if(LogStatus(google::protobuf::util::MessageToJsonString(response, &details, opts)).ok())
        {
            cout << details << endl;
        }
    }
}

void SiteAgentCtl::GetKey(remote::IKey::Stub* siteA, const std::string& destination)
{
    using namespace std;
    using namespace grpc;
    using google::protobuf::Empty;

    ClientContext ctx;
    remote::KeyRequest request;
    remote::SharedKey response;

    URI destUri(destination);
    if(destUri.GetScheme() == "pkcs11")
    {
        LOGDEBUG("Using PKCS11 url");
        map<string, string> dictionary;
        destUri.ToDictionary(dictionary);
        auto objectIt = dictionary.find("object");
        auto keyIdIt = dictionary.find("id");
        if(objectIt != dictionary.end())
        {
            request.set_siteto(URI::Decode(objectIt->second));
            LOGDEBUG("Found destination: " + request.siteto());
        }
        if(keyIdIt != dictionary.end())
        {
            KeyID keyId = 0;
            try
            {
                keyId = strtoul(keyIdIt->second.c_str(), nullptr, 16);
                request.set_keyid(keyId);
                LOGDEBUG("Found key id: " + to_string(keyId));
            }
            CATCHLOGERROR
        }
    }
    else
    {
        request.set_siteto(destination);
    }

    LOGDEBUG("Getting key for " + destination);
    if(LogStatus(siteA->GetSharedKey(&ctx, request, &response)).ok())
    {
        const PSK keyValue(response.keyvalue().begin(), response.keyvalue().end());
        cout << "PKCS=" + response.url() + " Id=0x" + ToHexString(response.keyid()) + " Value=" + keyValue.ToString() << endl;
    }
}

int SiteAgentCtl::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution && definedArguments.IsSet(Names::generate))
    {
        size_t numberKeys = 0;
        definedArguments.GetProp(Names::generate, numberKeys);

        if(backingStores.size() == siteIds.size() && backingStores.size() >= 2)
        {
            keygen::Utils::KeyStores stores;
            for(size_t index = 0; index < backingStores.size(); index++)
            {
                stores.emplace_back(siteIds[index], keygen::BackingStoreFactory::CreateBackingStore(backingStores[index]));
            }

            // build key between every combination of stores
            if(!keygen::Utils::PopulateRandom(stores, numberKeys))
            {
                LOGERROR("Failed to populate keystores");
                stopExecution = true;
                exitCode = ExitCodes::UnknownError;
            }
        }
        else
        {
            LOGERROR("must specify at least 2 equal number of backing stores and ids");
            stopExecution = true;
            exitCode = ExitCodes::InvalidConfig;
        }
    }

    if(!stopExecution && definedArguments.IsSet(Names::connect))
    {

        GrpcAllowMACOnlyCiphers();

        // setup the credentials
        creds.set_usetls(definedArguments.IsSet(Names::tls));
        definedArguments.GetProp(Names::certFile, *creds.mutable_certchainfile());
        definedArguments.GetProp(Names::keyFile, *creds.mutable_privatekeyfile());
        definedArguments.GetProp(Names::rootCaFile, *creds.mutable_rootcertsfile());

        string siteAAddress = definedArguments.GetStringProp(Names::connect);
        auto channel = grpc::CreateChannel(siteAAddress, LoadChannelCredentials(creds));
        if(channel)
        {
            auto siteA = remote::ISiteAgent::NewStub(channel);
            auto siteAKey = remote::IKey::NewStub(channel);
            if(siteA)
            {
                for(auto& command : commands)
                {
                    switch (command.cmd)
                    {
                    case Command::Cmd::List:
                        ListSites(siteAKey.get());
                        break;
                    case Command::Cmd::Start:
                        StartNode(siteA.get(), command.physicalPath);
                        break;
                    case Command::Cmd::Stop:
                        StopNode(siteA.get(), command.physicalPath);
                        break;
                    case Command::Cmd::Details:
                        GetDetails(siteA.get());
                        break;
                    case Command::Cmd::Key:
                        GetKey(siteAKey.get(), command.destination);
                        break;
                    } // switch command

                } // for commands
            } // if siteA
        } // if channel

    } // if !stopExecution

    return exitCode;
}

CQP_MAIN(SiteAgentCtl)
