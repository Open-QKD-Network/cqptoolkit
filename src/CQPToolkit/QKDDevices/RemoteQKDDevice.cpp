/*!
* @file
* @brief RemoteQKDDevice
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/2/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "RemoteQKDDevice.h"
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "CQPToolkit/Interfaces/ISessionController.h"
#include "QKDInterfaces/Device.pb.h"
#include <grpcpp/create_channel.h>
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "Algorithms/Net/DNS.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include <thread>

namespace cqp
{
    using grpc::Status;
    using grpc::StatusCode;

    RemoteQKDDevice::RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                                     std::shared_ptr<grpc::ServerCredentials> creds) :
        device(device),
        creds(creds)
    {
    }

    RemoteQKDDevice::~RemoteQKDDevice()
    {
        shutdown = true;
        recievedKeysCv.notify_all();
        UnregisterWithSiteAgent();
        deviceServer.reset();
        device.reset();
    }

    grpc::Status RemoteQKDDevice::RunSession(grpc::ServerContext*, const remote::SessionDetailsTo* request, google::protobuf::Empty*)
    {
        Status result;

        auto session = device->GetSessionController();
        if(session)
        {
            result = session->Connect(request->peeraddress());
        }
        else
        {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        // prepare the device
        if(result.ok() && device->Initialise(request->details()))
        {
            session->StartSession(request->details());
        }
        else
        {
            result = Status(StatusCode::FAILED_PRECONDITION, "Initialisation failed");
        }
        return result;
    }

    grpc::Status RemoteQKDDevice::WaitForSession(grpc::ServerContext*, const remote::LocalSettings* settings,
            ::grpc::ServerWriter<remote::RawKeys>* writer)
    {
        Status result;

        auto initialKey = std::make_unique<PSK>();
        initialKey->resize(settings->initialkey().size());
        initialKey->assign(settings->initialkey().begin(), settings->initialkey().end());
        device->SetInitialKey(move(initialKey));

        auto session = device->GetSessionController();

        if(session)
        {
            // Wait for keys to arrive and pass them on
            // nothing will happen until RunSession is called on one side
            ProcessKeys(writer);
        }
        else
        {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        return result;
    }

    grpc::Status RemoteQKDDevice::GetLinkStatus(grpc::ServerContext* context, const google::protobuf::Empty*,
            ::grpc::ServerWriter<remote::LinkStatus>* writer)
    {

        Status result;
        ISessionController* session = nullptr;
        if(device)
        {
            session = device ->GetSessionController();
        }

        if(session)
        {
            result = session->GetLinkStatus(context, writer);
        }
        else
        {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        return result;
    }

    void RemoteQKDDevice::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
    {
        using namespace std;
        {
            /*lock scope*/
            lock_guard<mutex> lock(recievedKeysMutex);
            recievedKeys.emplace_back(move(keyData));

        }/*lock scope*/

        recievedKeysCv.notify_one();
    }

    grpc::Status RemoteQKDDevice::RegisterWithSiteAgent(const std::string& address)
    {
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        auto siteAgent = remote::ISiteAgent::NewStub(channel);
        grpc::ClientContext ctx;
        google::protobuf::Empty response;

        remote::ControlDetails request;
        (*request.mutable_config()) = device->GetDeviceDetails();
        request.set_controladdress(qkdDeviceAddress);
        auto result = siteAgent->RegisterDevice(&ctx, request, &response);

        if(result.ok())
        {
            siteAgentAddress = address;
        }

        return result;
    }

    void RemoteQKDDevice::UnregisterWithSiteAgent()
    {
        if(!siteAgentAddress.empty())
        {
            auto channel = grpc::CreateChannel(siteAgentAddress, grpc::InsecureChannelCredentials());
            auto siteAgent = remote::ISiteAgent::NewStub(channel);
            grpc::ClientContext ctx;
            google::protobuf::Empty response;
            remote::DeviceID id;
            id.set_id(device->GetDeviceDetails().id());

            LogStatus(siteAgent->UnregisterDevice(&ctx, id, &response));
            siteAgentAddress.clear();
        }
    }

    bool RemoteQKDDevice::StartControlServer(const std::string& controlAddress, const std::string& siteAgent)
    {
        // TODO reconsile the device config type with the need to provide this - the device doesn't inherently know it
        qkdDeviceAddress = controlAddress;
        siteAgentAddress = siteAgent;
        auto config = device->GetDeviceDetails();

        auto session = device->GetSessionController();
        // start the session server
        URI sessionAddress = config.sessionaddress();
        auto hostnameTobind = sessionAddress.GetHost();
        if(hostnameTobind.empty())
        {
            hostnameTobind = net::AnyAddress;
            sessionAddress.SetHost(net::GetHostname());
        }
        // start listening for session control on the provided address
        LogStatus(session->StartServer(hostnameTobind, sessionAddress.GetPort(), creds));
        // get the actual address for others to use for connections
        (*config.mutable_sessionaddress()) = session->GetConnectionAddress();

        // now start the IDevice server
        grpc::ServerBuilder devServBuilder;
        int listenPort {0};
        devServBuilder.AddListeningPort(controlAddress, creds, &listenPort);
        devServBuilder.RegisterService(this);

        deviceServer = devServBuilder.BuildAndStart();

        URI controlURI{controlAddress};
        controlURI.SetPort(listenPort);
        if(controlURI.GetHost().empty() || controlURI.GetHost() == net::AnyAddress)
        {
            controlURI.SetHost(net::GetHostname());
        }
        qkdDeviceAddress = controlURI.ToString();
        LOGINFO("Control interface available on " + qkdDeviceAddress);

        if(!siteAgentAddress.empty())
        {
            grpc::Status regResult;
            do
            {
                LOGINFO("Registering with site agent " + siteAgentAddress);
                regResult = LogStatus(RegisterWithSiteAgent(siteAgentAddress));
                if(!regResult.ok())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                }
            }
            while(!regResult.ok());
        }

        return deviceServer != nullptr;
    }

    void RemoteQKDDevice::WaitForServerShutdown()
    {
        if(deviceServer)
        {
            deviceServer->Wait();
        }
    }

    void RemoteQKDDevice::StopServer()
    {
        if(deviceServer)
        {
            deviceServer->Shutdown();
        }
    }

    grpc::Status RemoteQKDDevice::ProcessKeys(::grpc::ServerWriter<remote::RawKeys>* writer)
    {
        using namespace std;
        grpc::Status result;
        auto keyPub = device->GetKeyPublisher();
        if(keyPub)
        {
            keyPub->Attach(this);

            // send the key to the caller
            do
            {
                KeyListList tempKeys;

                {
                    /* lock scope*/
                    unique_lock<mutex> lock(recievedKeysMutex);
                    recievedKeysCv.wait(lock, [&]()
                    {
                        return !recievedKeys.empty() || shutdown;
                    });

                    if(!recievedKeys.empty())
                    {
                        tempKeys.resize(recievedKeys.size());
                        // move the data to our scope to allow the callback to carry on filling it up.
                        move(recievedKeys.begin(), recievedKeys.end(), tempKeys.begin());
                        recievedKeys.clear();
                    }
                }/* lock scope*/

                if(!shutdown)
                {
                    remote::RawKeys message;

                    int totalNumKeys = 0u;
                    // count the keys to reserve space
                    for(const auto& keylist : tempKeys)
                    {
                        totalNumKeys += keylist->size();
                    }
                    message.mutable_keydata()->Reserve(totalNumKeys);

                    for(const auto& list : tempKeys)
                    {
                        for(const auto& key : *list)
                        {
                            message.add_keydata(key.data(), key.size());
                        }
                    }

                    // send the data
                    writer->Write(message);
                } // if !shutdown
            } // do
            while(!shutdown);

            keyPub->Detatch();
        } // if keyPub
        else
        {
            result = Status(StatusCode::INTERNAL, "Invalid key publisher");
        }

        return result;
    }

} // namespace cqp
