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
#include "SDNLink.h"

#include "Algorithms/Logging/Logger.h"
#include <google/protobuf/util/json_util.h>
#include <QKDInterfaces/Polatis.pb.h>

namespace cqp
{
#if !defined(DOXYGEN)
    const std::string mediaTypeJSON = "application/json";
    const std::string mediaTypeText = "text/plain";

    /// Command paths for http requests
    struct
    {
        const std::string DumpTopo = "/dump_topology";
        const std::string DumpLinks = "/dump_links";
        const std::string CreateLink = "/create_link";
        const std::string DeleteLink = "/delete_link";
    } Commands;

    struct
    {
        const std::string Direction = "direction";
        const std::string NoDirection = "nodirection";
    } LinkTypes;

    struct
    {
        const std::string Origin = "origin";
        const std::string Destination = "destination";
        const std::string Type = "type";
        const std::string Active = "active";
        const std::string Id = "id";
    } Parameters;
#endif

    SDNLink* SDNLink::CreateLink(const URI& sdnControllerAddress, const std::string& from, const std::string& to)
    {
        // create a new object
        SDNLink* result = new SDNLink(sdnControllerAddress, from, to);

        // connect to the server
        if(!result->Connected())
        {
            LOGERROR("Failed to connect to SDN Controller");
            delete result;
            result = nullptr;
        }

        if(result)
        {
            // try to send the connection request
            if(!result->CreateLink())
            {
                LOGERROR("Failed to create link");
                // the request failed so don't return the object
                delete result;
                result = nullptr;
            }
        }

        return result;
    }

    bool SDNLink::BuildExistingLinks(const URI& sdnControllerAddress, List& linkList)
    {
        using namespace std;

        using namespace google::protobuf::util;
        // Create an instance to perform the communications
        SDNLink tempLink(sdnControllerAddress, "", "");

        // parse the response from the server
        polatis::Links links;
        bool result = JsonStringToMessage(tempLink.GetLinks(), &links).ok();

        if(result)
        {
            // walk through the results creating objects for the entries.
            for(const auto& link : links.active())
            {
                LOGDEBUG("Attaching to existing link: " + std::to_string(link.id()));
                SDNLink* attachedLink = new SDNLink(sdnControllerAddress,
                                                    link.origin(), link.destination());
                attachedLink->id = link.id();
                linkList.push_back(unique_ptr<SDNLink>(attachedLink));
            }
        }

        return result;
    }

    std::string SDNLink::GetLinks()
    {
        using namespace std;
        using cqp::net::HTTPRequest;
        using cqp::net::HTTPResponse;
        std::string result;

        // send a request to the server for a dump of the currently active links
        HTTPRequest request(HTTPRequest::Get);
        theController.SetAddress(controllerAddress.GetPath() + Commands.DumpLinks);
        LOGDEBUG("Sending request: " + theController.GetAddress().ToString());

        HTTPResponse response;
        theController.SendRequest(request, response);

        // read the response and return it as is
        if(response.status == HTTPResponse::Ok)
        {
            // copy the json string to the return value.
            result = response.body;
            LOGDEBUG("Get Links responded with: " + result);
        }
        else
        {
            LOGERROR(response.reason);
        }

        return result;
    }

    SDNLink::~SDNLink()
    {
        if(id != 0)
        {
            // We are attached to an active link, delete the link.
            if(!DeleteLink())
            {
                LOGWARN("Failed to delete link: " + names.first + " => " + names.second);
            }
        }
    }

    SDNLink::SDNLink(const URI& sdnControllerAddress, const std::string& from, const std::string& to) :
        controllerAddress(sdnControllerAddress)
    {
        names.first = from;
        names.second = to;

        theController.SetAddress(controllerAddress);

    }

    bool SDNLink::Connected()
    {
        using namespace std;
        using cqp::net::HTTPRequest;
        using cqp::net::HTTPResponse;

        bool result = theController.IsConnected();
        if(!result)
        {
            HTTPRequest request(HTTPRequest::Get);
            HTTPResponse response;
            theController.SetAddress(controllerAddress.GetPath() + Commands.DumpLinks);
            theController.SendRequest(request, response);

            if(response.status == HTTPResponse::Ok)
            {
                result = true;
            }
            else
            {
                LOGERROR(response.reason);
            }
        }

        return result;
    }

    bool SDNLink::CreateLink()
    {
        using namespace std;
        using cqp::net::HTTPRequest;
        using cqp::net::HTTPResponse;
        bool result = false;
        // remove any preexisting link
        DeleteLink();

        // build the request which will create the link

        // build the request to the server
        HTTPRequest request(HTTPRequest::Post, HTTPRequest::HTTP_1_1);
        request.contentType = mediaTypeJSON;
        request.parameters.push_back({"Accept", mediaTypeJSON});
        request.keepAlive = true;


        request.body = "{ " + Parameters.Origin + ": " + names.first + ", " +
                       Parameters.Destination + ": " + names.second + ", " +
                       Parameters.Type + ": " + LinkTypes.Direction + "}";

        LOGDEBUG("Sending request: " + theController.GetAddress().ToString() + "\nBody: " + request.body);
        theController.SetAddress(controllerAddress.GetPath() + Commands.CreateLink);
        HTTPResponse response;
        // sendRequest returns a ostream which we can send the body text with
        theController.SendRequest(request, response);

        if(response.status == HTTPResponse::Ok)
        {
            // get the link ids from the response
            try
            {
                id = stol(response.body);
                result = true;
                LOGDEBUG("Created link " + to_string(id) + " : " + response.reason);
            }
            catch (const exception& e)
            {
                LOGERROR(response.body + " : " + response.reason);
                id = 0;
            }
        }
        else
        {
            LOGERROR(response.reason);
        }

        return result;
    }

    bool SDNLink::DeleteLink()
    {
        using namespace std;
        using cqp::net::HTTPRequest;
        using cqp::net::HTTPResponse;
        bool result = false;

        if(id != 0)
        {
            // build the request which will delete the link
            HTTPRequest request(HTTPRequest::Delete, HTTPRequest::HTTP_1_1);
            HTTPResponse response;
            theController.SetAddress(controllerAddress.GetPath() + Commands.DeleteLink);
            LOGDEBUG("Sending request: " + theController.GetAddress().ToString());

            request.contentType = mediaTypeJSON;
            request.parameters.push_back({"Accept", mediaTypeJSON});
            request.keepAlive = true;

            request.body = "\"" + to_string(id) + "\"";

            // build the request to the server
            theController.SendRequest(request, response);

            if(response.status == HTTPResponse::Ok)
            {
                result = true;
                id = 0;
                LOGDEBUG("Link deleted.");
            }
            else
            {
                LOGERROR(response.reason);
            }

        }
        else
        {
            // nothing to do
            result = true;
        }

        return result;
    }
}
