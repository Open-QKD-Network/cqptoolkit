/*!
* @file
* @brief NetworkManager
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 2/4/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "NetworkManager.h"
#include <grpcpp/create_channel.h>
#include "QKDInterfaces/ISiteAgent.grpc.pb.h"
#include "Algorithms/Logging/Logger.h"
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{

    using grpc::Status;
    using grpc::StatusCode;

    NetworkManager::NetworkManager(google::protobuf::RepeatedPtrField<remote::PhysicalPath> staticPaths,
                                   std::shared_ptr<grpc::ChannelCredentials> creds) :
        creds{creds}
    {
        for(const auto& path : staticPaths)
        {
            // dont store empty links
            if(!path.hops().empty())
            {
                links.emplace_back(Link(std::move(path)));
            } // if link !empty
        } // for static paths
    }

    NetworkManager::~NetworkManager()
    {
        StopActiveLinks();
    }

    void NetworkManager::StopActiveLinks()
    {
        std::lock_guard<std::mutex> lock(sitesMutex);

        for (auto& link : links)
        {
            if(link.active)
            {
                StopLink(link);
            }
        }
    }

    void NetworkManager::StartLink(Link &link)
    {
        auto channel = grpc::CreateChannel(link.path.hops(0).first().site(), creds);
        auto stub = remote::ISiteAgent::NewStub(channel);
        grpc::ClientContext ctx;
        google::protobuf::Empty response;

        if(LogStatus(stub->StartNode(&ctx, link.path, &response)).ok())
        {
            // record the hop as active
            link.active = true;
        }
    } // StartLink

    void NetworkManager::StopLink(Link &link)
    {
        auto channel = grpc::CreateChannel(link.path.hops(0).first().site(), creds);
        auto stub = remote::ISiteAgent::NewStub(channel);
        grpc::ClientContext ctx;
        google::protobuf::Empty response;

        if(LogStatus(stub->EndKeyExchange(&ctx, link.path, &response)).ok())
        {
            // record the hop as active
            link.active = false;
        }
    } // StopLink

    grpc::Status NetworkManager::RegisterSite(grpc::ServerContext*, const remote::Site* request, google::protobuf::Empty*)
    {
        Status result;
        LOGINFO("Site " + request->url() + " registering with " + std::to_string(request->devices().size()) + " devices");

        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(sitesMutex);

            // update the detials
            sites[request->url()] = *request;

            // check all the links
            for(auto& link : links)
            {
                auto thisSite = link.sites.find(request->url());
                if(thisSite != link.sites.end())
                {
                    // we are involved in this link
                    // check all the devices that are required from this site

                    for(auto& requiredDevice : thisSite->second)
                    {
                        // set the ready flag, if a device is removed it will be set to false
                        requiredDevice.second = std::find_if(request->devices().begin(), request->devices().end(), [&](const auto& device)
                        {
                            return device.config().id() == requiredDevice.first;

                        }) != request->devices().end();
                    } // for required devices

                    CheckLink(link);
                    // check if this link is ready to go or needs to be stopped

                } // if site

            } // for link


        } /*lock scope*/

        return result;
    }

    void NetworkManager::CheckLink(Link& link)
    {

        auto allDevicesReady = true;
        for(const auto& site : link.sites)
        {
            allDevicesReady &= std::all_of(site.second.begin(), site.second.end(), [](auto ready)
            {
                return ready.second;
            });

            if(!allDevicesReady)
            {
                break; // for sites
            }
        } // for sites

        if(allDevicesReady && !link.active)
        {
            // all the devices are now available
            StartLink(link);
        } // if inactive
        else if(link.active && !allDevicesReady)
        {
            StopLink(link);
        } // else if active
    } // CheckLink

    grpc::Status NetworkManager::UnregisterSite(grpc::ServerContext*, const remote::SiteAddress* request, google::protobuf::Empty*)
    {
        Status result;

        LOGINFO("Site " + request->url() + " unregistering");

        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(sitesMutex);

            auto foundSite = sites.find(request->url());
            if(foundSite != sites.end())
            {
                sites.erase(foundSite);

                // check all the links
                for(auto& link : links)
                {
                    auto thisSite = link.sites.find(request->url());
                    if(thisSite != link.sites.end())
                    {
                        // we are involved in this link
                        // check all the devices that are required from this site

                        for(auto requiredDevice : thisSite->second)
                        {
                            // clear the ready flag for devices for this site
                            requiredDevice.second = false;
                        } // for required devices

                        StopLink(link);
                    } // if site
                } // for link
            }
            else
            {
                result = Status(StatusCode::NOT_FOUND, "Site not registered");
            }
        }/*lock scope*/

        return result;
    }

    grpc::Status NetworkManager::GetRegisteredSites(grpc::ServerContext*, const google::protobuf::Empty*, remote::SiteDetailsList* response)
    {
        /*lock scope*/
        {
            std::lock_guard<std::mutex> lock(sitesMutex);

            for(auto site : sites)
            {
                (*response->add_sites()) = site.second;
            }
        }/*lock scope*/

        return Status();
    }

    NetworkManager::Link::Link(const remote::PhysicalPath& pathIn) :
        path{pathIn}
    {
        for(auto& hop : path.hops())
        {
            for(auto& side :
                    {
                        hop.first(), hop.second()
                    })
            {
                // store the placeholder for the required device
                sites[side.site()][side.deviceid()] = false;
            } // for side
        } // for hop
    } // Link

} // namespace cqp
