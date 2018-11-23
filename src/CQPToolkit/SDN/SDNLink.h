/*!
* @file
* @brief SDNLink
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/3/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Platform.h"
#include <memory>
#include "CQPToolkit/Net/HTTPClientSession.h"
#include "CQPToolkit/Util/URI.h"

namespace cqp
{
    /// Manages SDN links
    class CQPTOOLKIT_EXPORT SDNLink
    {
    public:
        /// A list of managed links
        using List = std::vector<std::unique_ptr<SDNLink>>;

        /// Create a managed link
        /// @details The class will delete the link when it is destroyed
        /// @param sdnControllerAddress The controller to issue commands to
        /// @param from The identifier for the first end point
        /// @param to The identifier for the second end point
        /// @returns Ether a pointer to a valid link if successful or null if it failed to create the link
        static SDNLink* CreateLink(const URI& sdnControllerAddress,
                                   const std::string& from, const std::string& to);

        /// Build a list of objects based on the connections already present in the specified controller
        /// @param sdnControllerAddress The controller to issue commands to
        /// @param[in,out] linkList Storage for the created objects
        /// @returns whether the command was a success
        /// @todo Output from the command need to be fixed for this to work
        static bool BuildExistingLinks(const URI& sdnControllerAddress, List& linkList);

        /// Check the state of the connection to the controller
        /// @returns true if the connection is valid
        bool Connected();

        /// Send the create link command to the server
        /// @returns true if the command succeeded
        bool CreateLink();

        /// Send the delete link command to the server
        /// @returns true if the command succeeded
        bool DeleteLink();

        /// Destructor
        /// This will take down the link if it has been created
        virtual ~SDNLink();
    protected:
        /// Construct an object with the required parameters
        /// @param sdnControllerAddress The controller to issue commands to
        /// @param from The identifier for the first end point
        /// @param to The identifier for the second end point
        SDNLink(const URI& sdnControllerAddress, const std::string& from, const std::string& to);

        /// Returns the current topology of the system
        /// @note TBD, This currently returns the raw json returned from the server
        /// @returns json returned from the server
        std::string GetLinks();

        /// The address of the sdn controller
        URI controllerAddress;
        /// The socket which connects to the sdn controller for sending commands
        net::HTTPClientSession theController;
        /// The identifiers for the end points
        std::pair<std::string, std::string> names;
        /// The id returned by create link
        /// This is reset by DeleteLink()
        long long id {0};
    };

}
