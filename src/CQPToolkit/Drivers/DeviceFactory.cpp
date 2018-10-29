/*!
* @file
* @brief DeviceFactory
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DeviceFactory.h"
#include <memory>                                      // for allocator, __s...
#include <utility>                                     // for move, pair
#include "CQPToolkit/Interfaces/IQKDDevice.h"          // for IQKDDevice
#include "CQPToolkit/Interfaces/ISessionController.h"  // for ISessionContro...
#include "CQPToolkit/Statistics/StatCollection.h"      // for StatCollection
#include "CQPToolkit/Util/Logger.h"                    // for LOGERROR, LOGT...
#include "CQPToolkit/Util/Util.h"                      // for StrEqualI
#include "QKDInterfaces/Site.pb.h"                     // for Side, Side::Any
#include "Util/URI.h"                                  // for URI

namespace cqp
{
    // storage for the class member
    std::unordered_map<std::string /*driver name*/, DeviceFactory::DeviceCreateFunc> DeviceFactory::driverMapping;


    DeviceFactory::DeviceFactory(std::shared_ptr<grpc::ChannelCredentials> creds) :
        clientCreds(creds)
    {
    }

    std::string DeviceFactory::GetDeviceIdentifier(const std::shared_ptr<IQKDDevice> device)
    {
        return GetDeviceIdentifier(device->GetAddress());
    }

    std::string DeviceFactory::GetDeviceIdentifier(const URI& url)
    {
        using namespace std;
        string switchPort;
        string side;
        size_t bytesPerKey = DefaultBytesPerKey;

        url.GetFirstParameter(IQKDDevice::Parmeters::switchPort, switchPort);
        url.GetFirstParameter(IQKDDevice::Parmeters::side, side);
        url.GetFirstParameter(IQKDDevice::Parmeters::keybytes, bytesPerKey);

        std::string result = url.GetScheme() + "_" +
                             url.GetHost() + "_" +
                             std::to_string(url.GetPort()) + "_" +
                             switchPort + "_" + std::to_string(bytesPerKey) + "_" +
                             side;

        return result;
    }

    std::shared_ptr<IQKDDevice> DeviceFactory::CreateDevice(const std::string& url)
    {
        using namespace std;
        URI addrUri(url);
        std::shared_ptr<IQKDDevice> result;

        auto CreateFunc = driverMapping[addrUri.GetScheme()];
        size_t bytesPerKey = DefaultBytesPerKey;
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::keybytes, bytesPerKey);

        if(CreateFunc)
        {
            LOGTRACE("Calling create for device");
            result = CreateFunc(url, clientCreds, bytesPerKey);
            if(result)
            {
                LOGTRACE("Created device");
                lock_guard<mutex> lock(changeMutex);

                const std::string identifier = GetDeviceIdentifier(addrUri);
                allDevices[identifier] = result;
                unusedDevices[identifier] = result;

                // link the reporting callbacks
                auto controller = result->GetSessionController();
                if(controller)
                {
                    LOGTRACE("Collecting device statistics");
                    for(auto* collection : controller->GetStats())
                    {
                        for(auto reportCb : reportingCallbacks)
                        {
                            collection->Add(reportCb);
                        }
                    }
                    LOGINFO("Device " + addrUri.GetScheme() + " ready");
                }
                else
                {
                    LOGERROR("Invalid controller");
                    result = nullptr;
                }
            }
        }
        else
        {
            LOGERROR("Unknown device: " + url);
        }

        return result;
    }

    std::shared_ptr<IQKDDevice> DeviceFactory::UseDeviceById(const std::string& identifier)
    {
        using namespace std;
        shared_ptr<IQKDDevice> result;

        lock_guard<mutex> lock(changeMutex);

        auto devIt = unusedDevices.find(identifier);
        if(devIt == unusedDevices.end())
        {
            URI addrUri(identifier);
            devIt = unusedDevices.find(GetDeviceIdentifier(addrUri));
        }

        if(devIt != unusedDevices.end())
        {
            result = devIt->second;
            // mark the device as in use.
            unusedDevices.erase(devIt->first);
        }

        return result;
    }

    void DeviceFactory::ReturnDevice(std::shared_ptr<IQKDDevice> device)
    {
        using namespace std;
        const string devId = GetDeviceIdentifier(device);

        lock_guard<mutex> lock(changeMutex);
        if(allDevices[devId] != nullptr)
        {
            unusedDevices[devId] = device;
        }
        else
        {
            LOGERROR("Device does not belong to this factory.");
        }
    }

    void DeviceFactory::RegisterDriver(const std::string& name, DeviceFactory::DeviceCreateFunc createFunc)
    {
        driverMapping[name] = createFunc;
    }

    remote::Side::Type DeviceFactory::GetSide(const URI& uri)
    {
        remote::Side::Type result = remote::Side::Any;
        std::string whichSide;
        if(uri.GetFirstParameter(IQKDDevice::Parmeters::side, whichSide))
        {
            if(StrEqualI(whichSide, IQKDDevice::Parmeters::SideValues::alice))
            {
                result = remote::Side::Alice;
            }
            else if(StrEqualI(whichSide, IQKDDevice::Parmeters::SideValues::bob))
            {
                result = remote::Side::Bob;
            }
            else if(StrEqualI(whichSide, IQKDDevice::Parmeters::SideValues::any))
            {
                result = remote::Side::Any;
            }
        }
        return result;
    }

    void DeviceFactory::AddReportingCallback(stats::IAllStatsCallback* callback)
    {
        reportingCallbacks.push_back(callback);
        for(auto dev : allDevices)
        {
            for(auto* collection : dev.second->GetSessionController()->GetStats())
            {
                collection->Add(callback);
            }
        }
    }

    void DeviceFactory::RemoveReportingCallback(stats::IAllStatsCallback* callback)
    {
        for(auto it = reportingCallbacks.begin(); it != reportingCallbacks.end(); it++)
        {
            if(*it == callback)
            {
                reportingCallbacks.erase(it);
                break; // for
            }
        }

        for(auto dev : allDevices)
        {
            for(auto* collection : dev.second->GetSessionController()->GetStats())
            {
                collection->Remove(callback);
            }
        }
    }

    std::vector<std::string> DeviceFactory::GetKnownDrivers()
    {
        std::vector<std::string> result;
        for(auto driver : driverMapping)
        {
            result.push_back(driver.first);
        }
        return result;
    }

} // namespace cqp
