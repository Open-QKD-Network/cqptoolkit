/*!
* @file
* @brief IEvent
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

namespace cqp
{
    /// Iterface for providing callbacks to known interfaces
    /// @tparam C The class which implements the callback function
    /// @tparam Data The type of data which will be passed to the callback
    /// @tparam Func The C member function which is to be called when the event occours
    template<class C>
    class IEvent
    {
    public:
        /// Attaches the callback to this event
        /// @param[in] listener the instance to be attached
        virtual void Add(C* const listener) = 0;

        /// Removes the callback from this event
        /// @param[in] listener the instance to be removed
        virtual void Remove(C* const listener) = 0;

        /// Removes any added listeners
        virtual void Clear() = 0;
    };
}
