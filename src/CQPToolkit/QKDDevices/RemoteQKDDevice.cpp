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

namespace cqp {
    using grpc::Status;
    using grpc::StatusCode;

    RemoteQKDDevice::RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                                     std::shared_ptr<grpc::ServerCredentials> creds) :
        device(device),
        creds(creds)
    {

        remote::DeviceConfig config;
        auto session = device->GetSessionController();
        // start the session
        sessionAddress = config.sessionaddress();
        auto hostnameTobind = sessionAddress.GetHost();
        if(hostnameTobind.empty())
        {
            hostnameTobind = net::AnyAddress;
            sessionAddress.SetHost(net::GetHostname());
        }
        uint16_t listenPort = sessionAddress.GetPort();
        LogStatus(session->StartServer(hostnameTobind, listenPort, creds));
        sessionAddress = session->GetConnectionAddress();
    }

    RemoteQKDDevice::~RemoteQKDDevice()
    {
        shutdown = true;
        recievedKeysCv.notify_all();
    }

    grpc::Status RemoteQKDDevice::RunSession(grpc::ServerContext*, const remote::SessionDetailsTo* request, google::protobuf::Empty*)
    {
        Status result;

        auto session = device->GetSessionController();
        if(session)
        {
            result = session->Connect(request->peeraddress());

        } else {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        // prepare the device
        if(result.ok() && device->Initialise(request->details()))
        {
            session->StartSession(request->details());
        } else {
            result = Status(StatusCode::FAILED_PRECONDITION, "Initialisation failed");
        }
        return result;
    }

    grpc::Status RemoteQKDDevice::WaitForSession(grpc::ServerContext*, const google::protobuf::Empty*,
                                                 ::grpc::ServerWriter<remote::RawKeys>* writer)
    {
        Status result;

        auto session = device->GetSessionController();

        if(session)
        {
            // Wait for keys to arrive and pass them on
            // nothing will happen until RunSession is called on one side
            ProcessKeys(writer);
        } else {
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
        } else {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        return result;
    }

    void RemoteQKDDevice::OnKeyGeneration(std::unique_ptr<KeyList> keyData)
    {
        using namespace std;
        {/*lock scope*/
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
        auto config = device->GetDeviceDetails();
        // TODO reconsile the device config type with the need to provide this - the device doesn't inherently know it
        config.set_controladdress(controlAddress);
        return siteAgent->RegisterDevice(&ctx, config, &response);
    }

    void RemoteQKDDevice::SetControlAddress(const std::string& address)
    {
        // TODO reconsile the device config type with the need to provide this - the device doesn't inherently know it
        controlAddress = address;
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
            do{
                KeyListList tempKeys;

                {/* lock scope*/
                    unique_lock<mutex> lock(recievedKeysMutex);
                    recievedKeysCv.wait(lock, [&](){
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
        else {
            result = Status(StatusCode::INTERNAL, "Invalid key publisher");
        }

        return result;
    }

} // namespace cqp
