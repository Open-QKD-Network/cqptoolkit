/*!
* @file
* @brief DummyQKD
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DummyQKD.h"
#include <utility>                                    // for move
#include "CQPToolkit/Drivers/DeviceFactory.h"         // for DeviceFactory
#include "CQPToolkit/Session/DummyAliceController.h"  // for DummyAliceContr...
#include "CQPToolkit/Session/DummyBobController.h"    // for DummyBobController
#include "Interfaces/IQKDDevice.h"                    // for IQKDDevice, IQK...
#include "Session/SessionController.h"                // for SessionController
#include "Algorithms/Logging/Logger.h"                              // for LOGERROR

namespace cqp
{
    class ISessionController;

    constexpr const char* DummyQKD::DriverName;

    void DummyQKD::RegisterWithFactory()
    {
        // tell the factory how to create a DummyQKD by specifying a lambda function
        DeviceFactory::RegisterDriver(DriverName, [](const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)
        {
            return std::shared_ptr<IQKDDevice>(new DummyQKD(address, creds, bytesPerKey));
        });
    }

    DummyQKD::DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)
    {
        remote::Side::Type side = DeviceFactory::GetSide(URI(address));
        myAddress = address;
        // create the controller for this device
        switch (side)
        {
        case remote::Side::Alice:
            controller = new session::DummyAliceController(creds, bytesPerKey);
            break;
        case remote::Side::Bob:
            controller = new session::DummyBobController(creds, bytesPerKey);
            break;
        default:
            LOGERROR("Invalid device side");
            break;
        }
    }

    DummyQKD::DummyQKD(const remote::Side::Type & side, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)
    {
        myAddress = std::string(DriverName) + ":///?side=";
        // create the controller for this device
        switch (side)
        {
        case remote::Side::Alice:
            myAddress += "alice";
            controller = new session::DummyAliceController(creds, bytesPerKey);
            break;
        case remote::Side::Bob:
            myAddress += "bob";
            controller = new session::DummyBobController(creds, bytesPerKey);
            break;
        default:
            LOGERROR("Invalid device side");
            break;
        }
    }

    std::string cqp::DummyQKD::GetDriverName() const
    {
        return DriverName;
    }

    URI cqp::DummyQKD::GetAddress() const
    {
        return myAddress;
    }

    bool cqp::DummyQKD::Initialise()
    {
        return true;
    }

    std::string DummyQKD::GetDescription() const
    {
        return "Fake QKD device for testing";
    }

    ISessionController* DummyQKD::GetSessionController()
    {
        return controller;
    }

    remote::Device DummyQKD::GetDeviceDetails()
    {
        remote::Device result;
        URI addrUri(myAddress);

        result.set_id(DeviceFactory::GetDeviceIdentifier(addrUri));
        std::string sideName;
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::side, sideName);
        result.set_side(DeviceFactory::GetSide(addrUri));
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchName, *result.mutable_switchname());
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchPort, *result.mutable_switchport());
        result.set_kind(addrUri.GetScheme());

        return result;
    }

} // namespace cqp

