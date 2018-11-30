/*!
* @file
* @brief CQP Toolkit - Event template
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <vector>
#include <future>
#include "CQPAlgorithms/Util/IEvent.h"
#include "CQPAlgorithms/Logging/Logger.h"
#include <functional>

namespace cqp
{

    /// The maximum number of listeners any publisher can have
    static const char* MaxListenersError = "Maximum number of listeners reached, new listener dropped.";

    /**
     * @brief
     * Template base class which provides basic Add/Remove functions
     * @details
     * Can be used to write custom Emit functions without rewriting event handling
     */
    template<class Interface, size_t MaxListeners = std::numeric_limits<std::size_t>::max()>
    class EventBase : public virtual IEvent<Interface>
    {
    public:

        /// Attaches the callback to this event
        /// @param listener The listener to add to event list
        virtual void Add(Interface* const listener) override
        {
            if(listeners.size() >= MaxListeners)
            {
                LOGERROR(MaxListenersError);
            }
            else if (listener != nullptr)
            {
                listeners.push_back(listener);
            }
        }

        /// Removes the callback from this event
        /// @param listener The listener to remove from the event list
        virtual void Remove(Interface* const listener) override
        {
            if (listener != nullptr)
            {
                // Not overly efficient but the number of listeners is likely to be low and
                // Removal from the list is unusual
                for (unsigned index = 0; index < listeners.size(); index++)
                {
                    if (listener != nullptr && (listeners[index] == listener))
                    {
                        listeners.erase(listeners.begin() + index);
                        break;
                    }
                }
            }
        }

        /// Removes any added listeners
        virtual void Clear() override
        {
            listeners.clear();
        }

    protected:
        /// Storage for the currently registered listeners
        typedef std::vector<Interface*> ListenerList;
        /// Stores the currently registered listeners
        ListenerList listeners;
    };

    /// @brief Event
    /// Standard mechanism for providing callbacks to known interfaces
    /// @details Create an implementation of this template to provide an event
    /// Example usage:
    /// @code{.cpp}   Event<void (IDetectorCallback::*)(const DetectorId&), &IDetectorCallback::OnDetection> detectorEvents; @endcode
    /// @tparam Interface The class which implements the callback function
    /// @tparam Args The type(s) of data which will be passed to the callback
    /// @tparam Func The Interface member function which is to be called when the event occurs
    /// @see http://stackoverflow.com/questions/36549237/template-parameter-function-pointer-with-variadic-arguments
    /// @note The Args... parameter allows multiple parameters to be passed to the template.
    template<class FuncSig, FuncSig, size_t MaxListeners = std::numeric_limits<std::size_t>::max()>
    class Event;

    /// @brief Event
    /// Standard mechanism for providing callbacks to known interfaces
    /// @warning For simplicity, the Interface parameter can be omitted.
    /// @details Create an implementation of this template to provide an event
    /// Example usage:
    /// @code{.cpp}   Event<Interface, void (IDetectorCallback::*)(const DetectorId&), &IDetectorCallback::OnDetection> detectorEvents; @endcode
    /// @tparam Interface The class which implements the callback function
    /// @tparam Args The type(s) of data which will be passed to the callback
    /// @tparam Func The Interface member function which is to be called when the event occurs
    /// @see http://stackoverflow.com/questions/36549237/template-parameter-function-pointer-with-variadic-arguments
    /// @note The Args... parameter allows multiple parameters to be passed to the template.
    template<class Interface, typename ...Args, void (Interface::*Func)(Args...), size_t MaxListeners>
    class Event<void (Interface::*)(Args...), Func, MaxListeners> :
        public EventBase<Interface, MaxListeners>
    {
    public:
        /// Construct a new event handler
        Event() { }
        virtual ~Event() {}

        /// Cause all registered callbacks to be notified with the data supplied
        /// @param args values to pass to the listeners
        virtual void Emit(Args... args) const
        {
            using namespace std;
#if defined(PARALLEL_EMIT)
            vector<future<void>> futures;
#endif

            for (Interface* cb : this->listeners)
            {
                // Allow the os to pass this to a thread pool if possible
                // Bind the template parameter function pointer to the object cb, then call the result
                // asynchronously

#if defined(PARALLEL_EMIT)
                futures.push_back(async(launch::async, [args..., cb]()
                {
#endif

                    // The data may have been created on the stack,
                    auto func = bind(Func, cb, args...);
                    // Call the bound function
                    __try
                    {
                        func();
                    }
                    CATCHLOGERROR

#if defined(PARALLEL_EMIT)
                }));
#endif

            }

#if defined(PARALLEL_EMIT)
            for (future<void>& fut : futures)
            {
                fut.wait();
            }
#endif
        }

    };
}
