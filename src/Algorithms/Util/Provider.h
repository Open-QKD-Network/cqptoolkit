/*!
* @file
* @brief Provider
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18 Sep 2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Logging/Logger.h"
#include <functional>
#include <utility>
#include <mutex>

namespace cqp
{

    /**
     * Simplifies the handling of one to one publisher subscriber interface
     */
    template<class Listener>
    class Provider
    {
    public:

        /// Constructor
        Provider() = default;

        /// destructor
        virtual ~Provider() = default;

        /**
         * @brief Attach
         * Set the listener
         * @param newListener
         */
        virtual void Attach(Listener* newListener)
        {
            std::unique_lock<std::mutex> lock(listenerMut);
            listener = newListener;
        }

        /**
         * @brief Detatch
         * Remove the listener
         */
        virtual void Detatch()
        {
            std::unique_lock<std::mutex> lock(listenerMut);
            listener = nullptr;
        }

        /**
         * Send the data to the listener
         * call by passing a function as the first parameter:
         * @code
         * Emit(&IInterface::Func, param1, move(uniqueptr));
         * @endcode
         * @tparam Args The parameters to send to the listener
         * @param args The values to send to the listener
         * @param func The function to call on the listener
         */
        template<typename ...Args>
        void Emit(void(Listener::*func)(Args...), Args... args)
        {
            using namespace std;
            std::unique_lock<std::mutex> lock(listenerMut);

            if(listener)
            {
                (listener->*func)(forward<Args>(args)...);
            }
            else
            {
                LOGWARN("No listener for data");
            }
        }

        /**
         * @brief HaveListener
         * @return true if listener is set
         */
        bool HaveListener()
        {
            std::unique_lock<std::mutex> lock(listenerMut);
            return listener != nullptr;
        }

    private:
        /// The listener
        Listener* listener = nullptr;
        /// control access to the listener
        std::mutex listenerMut;
    };

} // namespace cqp
