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
#include "Algorithms/Statistics/StatCollection.h"      // for StatCollection
#include "Algorithms/Logging/Logger.h"                    // for LOGERROR, LOGT...
#include "Algorithms/Util/Strings.h"                      // for StrEqualI
#include "QKDInterfaces/Site.pb.h"                     // for Side, Side::Any
#include "Algorithms/Datatypes/URI.h"                                  // for URI

namespace cqp
{
    // storage for the class member
    DeviceFactory::DriversBySide DeviceFactory::driverMapping;

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
        remote::Side::Type side = remote::Side::Type::Side_Type_Any;

        {
            string sideString;
            if(addrUri.GetFirstParameter(IQKDDevice::Parmeters::side, sideString))
            {
                sideString = ToLower(sideString);
                if(sideString == "alice" || sideString == "0")
                {
                    side = remote::Side::Alice;
                } else if(sideString == "bob" || sideString == "1")
                {
                    side = remote::Side::Bob;
                } else if(sideString == "any" || sideString == "2")
                {
                    side = remote::Side::Any;
                } else {
                    LOGERROR("Unknown side: " + sideString);
                }
            }
        }

        auto CreateFunc = driverMapping[side][addrUri.GetScheme()];
        size_t bytesPerKey = DefaultBytesPerKey;
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::keybytes, bytesPerKey);

        if(CreateFunc)
        {
            LOGTRACE("Calling create for " + addrUri.GetScheme());
            result = CreateFunc(url, clientCreds, bytesPerKey);
            if(result)
            {
                lock_guard<mutex> lock(changeMutex);

                const std::string identifier = GetDeviceIdentifier(addrUri);
                allDevices[identifier] = result;
                unusedDevices[identifier] = result;
                LOGTRACE("Device ready, collecting device statistics");
                // link the reporting callbacks
                for(auto* collection : result->GetStats())
                {
                    for(auto reportCb : reportingCallbacks)
                    {
                        collection->Add(reportCb);
                    }
                }
                LOGINFO("Device " + addrUri.GetScheme() + " ready");
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

    void DeviceFactory::RegisterDriver(const std::string& name, remote::Side::Type side, const DeviceFactory::DeviceCreateFunc& createFunc)
    {
        driverMapping[side][name] = createFunc;
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
        for(const auto& dev : allDevices)
        {
            for(auto* collection : dev.second->GetStats())
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

        for(const auto& dev : allDevices)
        {
            for(auto* collection : dev.second->GetStats())
            {
                collection->Remove(callback);
            }
        }
    }

    std::vector<std::string> DeviceFactory::GetKnownDrivers()
    {
        std::vector<std::string> result;
        result.resize(driverMapping.size());
        std::transform(driverMapping.begin(), driverMapping.end(), result.begin(), [](auto input){return input.first; });
        return result;
    }

} // namespace cqp
