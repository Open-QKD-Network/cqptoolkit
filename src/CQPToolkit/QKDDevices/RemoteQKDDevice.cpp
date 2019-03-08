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

namespace cqp {
    using grpc::Status;
    using grpc::StatusCode;

    RemoteQKDDevice::RemoteQKDDevice(std::shared_ptr<IQKDDevice> device,
                                     const remote::DeviceConfig& config,
                                     std::shared_ptr<grpc::ServerCredentials> creds) :
        device(device),
        config(std::make_unique<remote::DeviceConfig>(config)),
        creds(creds)
    {

    }

    grpc::Status RemoteQKDDevice::RunSession(grpc::ServerContext*, const remote::SessionDetails* request, google::protobuf::Empty*)
    {
        Status result;
        ISessionController* session = nullptr;
        if(device)
        {
            session = device ->GetSessionController();
        }

        if(session)
        {
            result = session->Connect(request->peeraddress());

        } else {
            result = Status(StatusCode::INTERNAL, "Invalid device/session objects");
        }

        return result;
    }

    grpc::Status RemoteQKDDevice::WaitForSession(grpc::ServerContext*, const google::protobuf::Empty*,
                                                 ::grpc::ServerWriter<remote::RawKeys>* writer)
    {
        Status result;
        ISessionController* session = nullptr;
        if(device)
        {
            session = device ->GetSessionController();
        }

        if(session)
        {
            // prepare the device
            if(device->Initialise(*config))
            {
                // start the session
                uint16_t listenPort = static_cast<uint16_t>(config->portnumber());
                result = session->StartServer(config->hostname(), listenPort, creds);

                // nothing to do but wait for the session to start
                if(result.ok())
                {
                    ProcessKeys(writer);
                }
            } else {
                result = Status(StatusCode::FAILED_PRECONDITION, "Initialisation failed");
            }
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

    void RemoteQKDDevice::ProcessKeys(::grpc::ServerWriter<remote::RawKeys>* writer)
    {
        using namespace std;
        auto keyPub = device->GetKeyPublisher();
        if(keyPub)
        {
            keyPub->Attach(this);
        }

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
            }
        }
        while(!shutdown);

        keyPub->Detatch();
    }

} // namespace cqp
