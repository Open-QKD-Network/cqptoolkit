/*!
* @file
* @brief AlignmentTestData
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestWeb.h"
#include "Algorithms/Net/DNS.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "KeyManagement/SDN/HTTPClientSession.h"

namespace cqp
{
    namespace tests
    {
        TEST(TestWeb, Parsing)
        {
            net::IPAddress ipAddress("255.172.123.0");
            ASSERT_TRUE(ipAddress.isIPv4);
            ASSERT_EQ(ipAddress.ip4[0], 255);
            ASSERT_EQ(ipAddress.ip4[1], 172);
            ASSERT_EQ(ipAddress.ip4[2], 123);
            ASSERT_EQ(ipAddress.ip4[3], 0);
        }

        TEST(TestWeb, DNS)
        {
            ASSERT_NE(net::GetHostname(), "");
            net::IPAddress ip;
            ASSERT_TRUE(net::ResolveAddress("localhost", ip));
            ASSERT_EQ(ip.ToString(), "127.0.0.1");
            ASSERT_TRUE(net::ResolveAddress("a.root-servers.net", ip));
            ASSERT_EQ(ip.ToString(), "198.41.0.4");

            auto myAddresses = net::GetHostIPs();
            for(auto addr : myAddresses)
            {
                std::cout << addr.ToString() << std::endl;
            }
            ASSERT_GT(myAddresses.size(), 0);
        }


        TEST(TestWeb, URITest)
        {
            URI uri;
            ASSERT_TRUE(uri.Parse("hostname:9000"));
            ASSERT_EQ(uri.GetHost(), "hostname");
            ASSERT_EQ(uri.GetPort(), 9000);

            ASSERT_TRUE(uri.Parse("http://www.google.com/?s=something"));
            std::string value;
            ASSERT_EQ(uri.GetScheme(), "http");
            ASSERT_EQ(uri.GetHost(), "www.google.com");
            ASSERT_TRUE(uri.GetFirstParameter("s", value));
            ASSERT_EQ("something", value);

            uri.AddParameter("joy", "happyness");
            ASSERT_TRUE(uri.GetFirstParameter("joy", value));
            ASSERT_EQ("happyness", value);

            uri.SetParameter("s", "else");
            ASSERT_TRUE(uri.GetFirstParameter("s", value));
            ASSERT_EQ("else", value);
        }

        TEST(TestWeb, Http)
        {
            using net::HTTPRequest;
            using net::HTTPResponse;
            using net::HTTPClientSession;

            HTTPClientSession session("http://www.google.co.uk");
            HTTPRequest request(HTTPRequest::RequestType::Get);
            HTTPResponse response;
            ASSERT_TRUE(session.SendRequest(request, response));
            ASSERT_EQ(response.status, HTTPResponse::Status::Ok);
        }
    } // namespace tests
} // namespace cqp
