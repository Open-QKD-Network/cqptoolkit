/*!
* @file
* @brief HTTPClientSession
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include <iostream>
#include "CQPToolkit/Util/URI.h"

namespace cqp
{
    namespace net
    {
        /**
         * @brief The HTTPMessage struct
         * Configuration of a message
         */
        struct CQPTOOLKIT_EXPORT HTTPMessage
        {
            /// Possible internet standards
            enum Standard {HTTP_1_0, HTTP_1_1};

            /// The internet standard for the message
            Standard standard = HTTP_1_0;

            /// The body of the message
            std::string body;
        };

        /**
         * @brief The HTTPRequest struct
         * A request message
         */
        struct CQPTOOLKIT_EXPORT HTTPRequest : public HTTPMessage
        {
            /// The kind of request
            enum RequestType { Get, Post, Delete };

            /**
             * @brief HTTPRequest
             * Constructor
             * @param type Which kind of message
             * @param standard Which HTTP standard to request
             */
            HTTPRequest(RequestType type, Standard standard = HTTP_1_0) :
                requestType(type)
            {
                this->standard = standard;
            }

            /// The kind of request
            RequestType requestType = Get;
            /// The mime type for the content
            std::string contentType;
            /// key, value parameters
            std::vector<std::pair<std::string, std::string>> parameters;
            /// should the socket use keepalive packets
            bool keepAlive = false;

            /**
             * @brief ReadCallback
             * Handles asynchronous requests for the body
             * @param buffer destination for body data
             * @param size max size of the buffer
             * @param nitems number of buffers (total bytes = nitems * size)
             * @param inStream pointer to this object
             * @return the number of bytes copied into buffer
             */
            static size_t ReadCallback(char* buffer, size_t size, size_t nitems, void* inStream);
        };

        /**
         * @brief The HTTPResponse struct
         * Response message from a server
         */
        struct CQPTOOLKIT_EXPORT HTTPResponse : public HTTPMessage
        {
            /// Code indicating the whether the request succeeded
            enum Status { Ok = 200 };

            /// whether the request succeeded
            Status status;
            /// more detail for the status
            std::string reason;

            /**
             * @brief WriteCallback
             * Handles asynchronous requests for the body of the message
             * @param buffer Source of the data to store in the body field
             * @param size number of bytes in buffer
             * @param nmemb number of buffers (total bytes = nmemb * size)
             * @param userdata pointer to this object
             * @return The number of bytes copied to the body field
             */
            static size_t WriteCallback(char* buffer, size_t size, size_t nmemb, void* userdata);
        };

        /**
         * @brief The HTTPClientSession class
         * Controls a session with a server
         */
        class CQPTOOLKIT_EXPORT HTTPClientSession
        {
        public:
            /**
             * @brief HTTPClientSession
             */
            HTTPClientSession();
            /**
             * @brief HTTPClientSession
             * @param address connection address
             */
            explicit HTTPClientSession(const URI& address);

            /// Destructor
            ~HTTPClientSession();

            /**
             * @brief SetAddress
             * Change the address of the server to connect to
             * @param address Server url
             */
            void SetAddress(const URI& address);

            /**
             * @brief GetAddress
             * @return The server address to connect to
             */
            URI GetAddress() const
            {
                return connectionAddress;
            }
            /**
             * @brief SendRequest
             * @param request
             * @param[out] response
             * @return stream for appending the body of the request
             */
            bool SendRequest(const HTTPRequest& request, HTTPResponse& response);

            /**
             * @brief IsConnected
             * @return true if the connection is open
             */
            bool IsConnected() const;

            /**
             * @brief Close
             * Close the connection to the server
             */
            void Close();
        protected:
            /// Implementation class
            struct Impl;
            /// Implementation class
            std::unique_ptr<Impl> impl;

            /// Address for the server
            URI connectionAddress;
        };

    } // namespace net
} // namespace cqp


