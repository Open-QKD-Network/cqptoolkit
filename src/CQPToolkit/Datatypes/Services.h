/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Datatypes/Base.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <unordered_set>
#include <algorithm>


namespace cqp
{

    /**
     * @brief The ServiceList class
     * A list of interfaces provided
     */
    class CQPTOOLKIT_EXPORT InterfaceList : public std::unordered_set<std::string>
    {
    public:
        /**
         * @brief contains
         * @param needle string to compare
         * @return true if needle exists in the list
         */
        bool contains(const std::string& needle) const
        {
            return std::any_of(begin(), end(), [needle](const std::string& str)
            {
                return str == needle;
            });
        }
    };

    /**
     * @brief The ServiceHost struct
     * Defines a connection point and the interfaces that are provided
     */
    struct CQPTOOLKIT_EXPORT RemoteHost
    {
        /// A user readable name
        std::string name;
        /// Unique identifier for the service provide, this can be used to identify
        /// a service provider between restarts on different ports
        std::string id;
        /// The current address for accessing the interface
        std::string host;
        /// port for interface
        uint16_t port;
        /// Interfaces provided by the host
        InterfaceList interfaces;

        /**
         * @brief operator ==
         * @param other the instance to compare
         * @return true if the contents of the instance are the same.
         */
        bool operator==(const RemoteHost& other) const
        {
            bool result = name == other.name && id == other.id && other.host == host &&
                          other.port == port &&
                          interfaces.size() == other.interfaces.size();
            if(result)
            {
                auto otherItt = other.interfaces.begin();
                for(auto myItt = interfaces.begin(); myItt != interfaces.end(); myItt++, otherItt++)
                {
                    result &= myItt == otherItt;
                }
            }
            return result;
        }


        /**
         * @brief operator !=
         * @param other the instance to compare
         * @return true if the contents of the instance are not the same.
         */
        bool operator!=(const RemoteHost& other) const
        {
            return !(*this == other);
        }

    };

    /// A list of RemoteHost
    using RemoteHosts = std::unordered_map<std::string, RemoteHost>;

}
