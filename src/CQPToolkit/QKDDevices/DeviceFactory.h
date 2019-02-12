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
#pragma once
#include <QKDInterfaces/Site.pb.h>  // for Side, Side::Type
#include <bits/shared_ptr.h>        // for shared_ptr
#include <bits/std_function.h>      // for function
#include <stddef.h>                 // for size_t
#include <mutex>                    // for mutex
#include <string>                   // for string
#include <unordered_map>            // for unordered_map
#include <vector>                   // for vector
#include "Algorithms/Datatypes/URI.h"    // for URI
#include "CQPToolkit/cqptoolkit_export.h"

namespace grpc
{
    class ChannelCredentials;
}

namespace cqp
{

    class IQKDDevice;

    namespace stats
    {
        class IAllStatsCallback;
    }

    /**
     * @brief The DeviceFactory class
     * Devices are identified by their driver and their address (schema and hostname portions of the full url)
     */
    class CQPTOOLKIT_EXPORT DeviceFactory
    {
    public:
        /// how to split up raw data into keys
        static const size_t DefaultBytesPerKey = 16;

        /**
         * @brief DeviceFactory
         * Constructor
         * @param creds Passed to the device when it is created
         */
        explicit DeviceFactory(std::shared_ptr<grpc::ChannelCredentials> creds);

        /**
         * @brief GetDeviceIdentifier
         * @param device
         * @return The device id
         */
        static std::string GetDeviceIdentifier(const std::shared_ptr<IQKDDevice> device);
        /**
         * @brief GetDeviceIdentifier
         * Do not assume anything about the construction of the string, always use
         * GetDeviceIdentifier to generate it.
         * @param url Address of the device
         * @return The device id
         */
        static std::string GetDeviceIdentifier(const URI& url);

        /**
         * @brief CreateDevice
         * @param url A url which defines a device
         * @return an instance of the device which matches the spec in the address
         */
        std::shared_ptr<IQKDDevice> CreateDevice(const std::string& url);

        /**
         * @brief UseDeviceByPort
         * @param identifier The url of the device
         * @return A device to handle the given address or nullptr if the address is invalid or the device is already in use
         */
        std::shared_ptr<IQKDDevice> UseDeviceById(const std::string& identifier);

        /**
         * @brief ReturnDevice
         * Mark a device as available for use after CreateDevice was called
         * @param device The device which is no longer being used
         */
        void ReturnDevice(std::shared_ptr<IQKDDevice> device);

        /// Function spec for creating a device object based on a url
        using DeviceCreateFunc = std::function<std::shared_ptr<IQKDDevice>(
            const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)>;

        /**
         * @brief RegisterDriver
         * @param name The driver name
         * @param createFunc Function which will create the device
         */
        static void RegisterDriver(const std::string& name, remote::Side::Type side, const DeviceCreateFunc& createFunc);

        /**
         * @brief GetSide
         * @param uri
         * @return The side (alice/bob) of the device
         */
        static remote::Side::Type GetSide(const URI& uri);

        /**
         * @brief AddReportingCallback
         * @param callback
         */
        void AddReportingCallback(stats::IAllStatsCallback* callback);
        /**
         * @brief RemoveReportingCallback
         * @param callback
         */
        void RemoveReportingCallback(stats::IAllStatsCallback* callback);

        /**
         * @brief GetKnownDrivers
         * @return A list of schemes for each registered driver
         */
        static std::vector<std::string> GetKnownDrivers();
    protected:
        /// A unqiue identifier for a driver
        using DriverNameList = std::unordered_map<std::string/*driver name*/, DeviceCreateFunc>;
        /// drivers stored by side
        using DriversBySide = std::unordered_map<remote::Side::Type, DriverNameList>;


        /// A list of all drivers
        static DriversBySide driverMapping;
        /// a list of all devices
        std::unordered_map<std::string /*driver+address*/, std::shared_ptr<IQKDDevice>> allDevices;
        /// devices which haven't been checked out with CreateDevice
        std::unordered_map<std::string /*driver+address*/, std::shared_ptr<IQKDDevice>> unusedDevices;
        /// control access to the lists
        std::mutex changeMutex;
        /// credentials passed to drivers when they are created
        std::shared_ptr<grpc::ChannelCredentials> clientCreds;
        /// all the callbacks to attach to owned devices
        std::vector<stats::IAllStatsCallback*> reportingCallbacks;
    };

} // namespace cqp


