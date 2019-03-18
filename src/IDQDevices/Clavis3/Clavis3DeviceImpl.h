/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Clavis3Device.h"
#include <zmq.hpp>
#include "SystemState.hpp"
#include <atomic>
#include <thread>
#include "QKDInterfaces/Site.pb.h"

namespace cqp
{

    class IDQDEVICES_NO_EXPORT Clavis3Device::Impl
    {
    public:

        Impl(const std::string& address);

        ~Impl();

        void Request_UpdateSoftware();

        void Request_Zeroize();

        void Request_SetInitialKey(DataBlock key);

        void Request_GetRandomNumber();

        void Request_GetProtocolVersion();

        void Request_GetSoftwareVersion();

        void Request_GetBoardInformation();

        void Request_PowerOn();

        void Request_PowerOff();

        void SubscribeToStateChange();

        bool ReadKey(PSK& keyValue);

        remote::Side::Type GetSide();

    protected: // methods

        void ReadSignalSocket();

    protected: // members

        static constexpr const uint16_t managementPort = 5561;
        static constexpr const uint16_t keyChannelPort = 5560;
        static constexpr const uint16_t signalsPort = 5562;

        zmq::context_t context {1};
        zmq::socket_t mgmtSocket{context, ZMQ_REQ};
        zmq::socket_t signalSocket{context, ZMQ_PAIR};
        zmq::socket_t keySocket{context, ZMQ_PAIR};

        std::atomic_bool shutdown {false};
        std::atomic<idq4p::domainModel::SystemState> state;
        std::thread signalReader;
    };

}
