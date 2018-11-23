/*!
* @file
* @brief ServiceDiscovery
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Platform.h"
#include "CQPToolkit/Util/Event.h"
#include <thread>
#include <unordered_map>
#include "CQPToolkit/Interfaces/IService.h"

namespace cqp
{
    namespace net
    {
        /// Event handling for service discovery
        using ServiceDiscoveryEvent = Event<void (IServiceCallback::*)(const RemoteHosts&, const RemoteHosts&), &IServiceCallback::OnServiceDetected>;

        class ServiceDiscoveryImpl;

        /**
         * @brief The ServiceDiscovery class
         * Detect and broadcast details of interfaces available on the network
         */
        class CQPTOOLKIT_EXPORT ServiceDiscovery : public ServiceDiscoveryEvent
        {
        public:
            /**
             * @brief ServiceDiscovery
             * Default constructor
             */
            ServiceDiscovery();

            /**
             * @brief AddService
             * Add a service description to broadcast to others
             * @param details The settings for the service
             */
            void SetServices(const RemoteHost &details);

            /**
             * @brief ~ServiceDiscovery
             */
            virtual ~ServiceDiscovery() override;

            /**
             * @brief GetServices
             * @return all services that have been discovered so far.
             */
            RemoteHosts GetServices() const
            {
                return services;
            }

            /// Attaches the callback to this event
            /// @param listener The listener to add to event list
            void Add(IServiceCallback* listener) override;

        protected:
            /// give the impl class access to protected
            friend ServiceDiscoveryImpl;
            /// implementation class to hide extraneous complexity
            ServiceDiscoveryImpl* impl = nullptr;

            /// Services which have been detected so far.
            RemoteHosts services;
            /// Services attached to this host
            std::vector<RemoteHost> myServices;
            /// protection for threading
            std::mutex changeMutex;

        }; // class ServiceDiscovery
    } // namespace net
} // namespace cqp


