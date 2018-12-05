/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Algorithms/Util/Event.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace cqp
{
    namespace tests
    {
        /// Dummy interface
        class IEventListener
        {
        public:
            /**
             * @brief Callback
             * @param data
             */
            virtual void Callback(const int& data) = 0;
            /**
             * @brief ~IEventListener
             */
            virtual ~IEventListener() = default;
        };

        /// Dummy data source
        class IEventSource : public IEvent<IEventListener>
        {

        };

        /// dummy listener
        class MockListener : public IEventListener
        {
        public:
            /**
             * @brief MockListener
             */
            MockListener() {}
            /**
             * @brief MOCK_METHOD1
             */
            MOCK_METHOD1(Callback, void(const int& data));
        };

        /// dummy event generator
        class MockEvent : public Event<void (IEventListener::*)(const int&), &IEventListener::Callback>
        {
        public:
            /**
             * @brief MockEvent
             */
            MockEvent() {}
            /**
             * @brief ~MockEvent
             */
            virtual ~MockEvent() {}
        };
    }
}
