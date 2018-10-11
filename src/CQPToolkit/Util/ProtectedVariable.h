/*!
* @file
* @brief ProtectedVariable
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 7/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <condition_variable>
#include <mutex>

namespace cqp
{
    /// A conditional_variable wrapper for protecting access to a single property
    template <typename T>
    class ProtectedVariable : protected std::condition_variable
    {
    public:
        /// Release a waiting thread
        /// @param[in] newData The new value to pass to store
        void notify_one(const T& newData)
        {
            std::lock_guard<std::mutex> lock(m);
            data = newData;
            changed = true;
            notify_one();
        }

        /// Block until a new value is stored by notify_one
        /// @param[in] rtime relative time to wait before giving up
        /// @param[out] out The new value if the wait was successful
        /// @return true if the wait sucessfull recieved a new value.
        template<typename _Rep, typename _Period>
        bool wait_for(const std::chrono::duration<_Rep, _Period>& rtime, T& out)
        {
            std::unique_lock<std::mutex> lock(m);
            bool result = std::condition_variable::wait_for(lock, rtime, [&] { return changed; });

            if(result)
            {
                changed = false;
                out = data;
            }
            return result;
        }

        /// Wait indefinately for a new value
        /// @return the new value provided by notify_one
        T wait()
        {
            std::unique_lock<std::mutex> lock(m);
            std::condition_variable::wait(lock, [&] { return changed; });
            changed = false;
            return data;
        }

        /**
         * @brief GetValue
         * @return the current value stored
         */
        T GetValue()
        {
            std::lock_guard<std::mutex> lock(m);
            return data;
        }

    protected:
        using condition_variable::notify_one;
        using condition_variable::wait;
        using condition_variable::wait_for;

        /// protect access to the variable
        std::mutex m;
        /// The data to store
        T data;
        /// Flag to detect when the data changes
        bool changed = false;
    };
}
