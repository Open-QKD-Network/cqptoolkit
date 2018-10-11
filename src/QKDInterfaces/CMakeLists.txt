### @file
### @brief CQP Toolkit - CQP Top level project
###
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
###
# Specify some settings which will be stored in the cache,
# these can be changed on the command line by adding -D<name>=<value>
# eg -DBUILD_TESTING=OFF
# Changing them in this file wont changed any values already stored in the cache

# See: https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.7.2)

# Specify how we expect cmake to behave
cmake_policy(VERSION 3.5)

project(QKDInterfaces CXX)

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

GRPC_PROJECT()

if(BUILD_SHARED)
    target_link_libraries(${PROJECT_NAME_SHARED} PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf)
endif(BUILD_SHARED)
