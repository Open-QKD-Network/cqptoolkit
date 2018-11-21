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
#include "HTTPClientSession.h"
#include <curl/curl.h>
#include "CQPToolkit/Util/Logger.h"

namespace cqp
{
    namespace net
    {
        CURLcode LogCurlResult(CURLcode code)
        {
            if(code != CURLE_OK)
            {
                LOGERROR(curl_easy_strerror(code));
            }
            return code;
        }

        struct HTTPClientSession::Impl
        {
            CURL* curl = curl_easy_init();

            Impl()
            {
                static CURLcode globalInitCode = curl_global_init(CURL_GLOBAL_ALL);
                LogCurlResult(globalInitCode);
            }

            ~Impl()
            {
                if(curl)
                {
                    // this causes double free?
                    //curl_free(curl);
                    curl = nullptr;
                }
            }
        };


        HTTPClientSession::HTTPClientSession() :
            impl(new Impl())
        {
        }

        HTTPClientSession::HTTPClientSession(const URI& address) :
            impl(new Impl()), connectionAddress(address)
        {
        }

        HTTPClientSession::~HTTPClientSession()
        {
            Close();
        }

        void HTTPClientSession::SetAddress(const URI& address)
        {
            connectionAddress = address;
        }

        bool HTTPClientSession::SendRequest(const HTTPRequest& request, HTTPResponse& response)
        {
            CURLcode curlResult {};
            curlResult = LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_URL, connectionAddress.ToString().c_str()));
            if(curlResult == CURLE_OK)
            {
                struct curl_slist *headers = nullptr;

                switch (request.standard)
                {
                case HTTPRequest::Standard::HTTP_1_0:
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_POSTFIELDSIZE, request.body.size()));
                    break;
                case HTTPRequest::Standard::HTTP_1_1:
                    headers = curl_slist_append(headers, "Transfer-Encoding: chunked");
                    break;
                }

                if(!request.contentType.empty())
                {
                    std::string contentTypeStr = "Content-Type: " + request.contentType;
                    headers = curl_slist_append(headers, contentTypeStr.c_str());
                }
                //TODO:
                switch (request.requestType)
                {
                case HTTPRequest::RequestType::Delete:
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_CUSTOMREQUEST, "DELETE"));
                    break;
                case HTTPRequest::RequestType::Get:
                    break;
                case HTTPRequest::RequestType::Post:
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_POST, 1));
                    break;
                }

                if(request.keepAlive)
                {
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_TCP_KEEPALIVE, 1));
                }

                LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_HTTPHEADER, headers));

                if(!request.body.empty())
                {
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_READFUNCTION, &HTTPRequest::ReadCallback));
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_READDATA, &request));
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_UPLOAD, 1));
                    LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_INFILESIZE_LARGE, request.body.size()));
                }

                // setup to receive response
                LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_WRITEFUNCTION, &HTTPResponse::WriteCallback));
                LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_WRITEDATA, &response));

                curlResult = LogCurlResult(curl_easy_perform(impl->curl));
                curl_slist_free_all(headers);

                LogCurlResult(curl_easy_getinfo(impl->curl, CURLINFO_RESPONSE_CODE, &response.status));

            }

            return curlResult == CURLE_OK;
        }

        bool HTTPClientSession::IsConnected() const
        {

            CURLcode curlResult {};
            LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_URL, connectionAddress.ToString().c_str()));
            LogCurlResult(curl_easy_setopt(impl->curl, CURLOPT_CONNECT_ONLY, 1));
            curlResult = LogCurlResult(curl_easy_perform(impl->curl));
            return curlResult == CURLE_OK;
        }

        void HTTPClientSession::Close()
        {
            curl_easy_cleanup(impl->curl);
        }

        size_t HTTPRequest::ReadCallback(char* buffer, size_t size, size_t nitems, void* inStream)
        {
            try
            {
                if(inStream)
                {
                    HTTPRequest* self = reinterpret_cast<HTTPRequest*>(inStream);
                    size_t bytesCopied = std::min(size * nitems, self->body.size());
                    self->body.copy(buffer, size * nitems);
                    self->body.erase(0, bytesCopied);
                    return bytesCopied;
                }
                else
                {
                    LOGERROR("Invalid user data in callback");
                    return 0;
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
                return CURL_READFUNC_ABORT;
            }

        }

        size_t HTTPResponse::WriteCallback(char* buffer, size_t size, size_t nmemb, void* userdata)
        {
            try
            {
                if(userdata)
                {
                    HTTPResponse* self = reinterpret_cast<HTTPResponse*>(userdata);

                    try
                    {

                        self->body.append(buffer, size * nmemb);

                        return size * nmemb;
                    }
                    catch (const std::exception& e)
                    {
                        LOGERROR(e.what());
                        return 0;
                    }
                }
                else
                {
                    LOGERROR("Invalid user data in callback");
                    return 0;
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
                return 0;
            }
        }

    } // namespace net
} // namespace cqp
