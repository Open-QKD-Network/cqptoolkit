/*!
* @file
* @brief TunnelTests
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/8/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <gtest/gtest.h>
#include <grpc++/server.h>

#include "KeyManagement/KeyStores/KeyStoreFactory.h"
#include "KeyManagement/KeyStores/KeyStore.h"

namespace cqp
{
    namespace tests
    {

        class TunnelTests : public testing::Test
        {
        public:
            TunnelTests();
            void SeedKeyStores();
        protected:
            int server1ListenPort = 0;
            int server2ListenPort = 0;

            keygen::KeyStoreFactory factory1;
            keygen::KeyStoreFactory factory2;

            std::shared_ptr<grpc::Server> keyServer1;
            std::shared_ptr<grpc::Server> keyServer2;
            std::shared_ptr<grpc::Server> farTunServer;

        };

    } // namespace tests
} // namespace cqp


