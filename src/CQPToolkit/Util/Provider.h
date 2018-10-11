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
#include "CQPToolkit/Util/Logger.h"
#include <functional>
namespace cqp {

    /**
     * Simplifies the handling of one to one publisher subscriber interface
     */
    template<class Listener>
    class Provider
    {
    public:
        virtual ~Provider() = default;


        virtual void Attach(Listener* newListener)
        {
            listener = newListener;
        }

        virtual void Detatch()
        {
            listener = nullptr;
        }

        template<typename ...Args>
        void Emit(void(Listener::*func)(Args...), Args&... args)
        {
            using namespace std;

            if(listener)
            {
                (listener->*func)(args...);
            }
        }

    protected:
        Provider() {}
        Listener* listener = nullptr;
    };

} // namespace cqp
