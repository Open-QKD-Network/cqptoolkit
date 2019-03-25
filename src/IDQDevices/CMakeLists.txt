### @file
### @brief IDQDevices - Drivers for IDQuantique devices
###
### @copyright Copyright (C) University of Bristol 2019
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 14 March 2019
### @author Richard Collins <richard.collins@bristol.ac.uk>
###
# Specify some settings which will be stored in the cache,
# these can be changed on the command line by adding -D<name>=<value>
# eg -DBUILD_TESTING=OFF
# Changing them in this file wont changed any values already stored in the cache

# See: https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.12.0)

if(TARGET IDQ4PClasses)
    project(IDQDevices CXX)

    # Perform some standard setup steps like detecting the platform.
    include(CommonSetup)

    ADD_GRPC_FILES()

    CQP_LIBRARY_PROJECT()

    target_compile_definitions(IDQDevices PRIVATE MSGPACK_DEFAULT_API_VERSION=1)
    target_compile_definitions(IDQDevices PRIVATE IDQ4P_DEBUG=true)
    target_compile_definitions(IDQDevices PRIVATE IDQ4P_ENABLE_SERIALIZER=true)

    #target_compile_definitions(IDQDevices PRIVATE MSGPACK_USE_DEFINE_MAP=true)
    #target_compile_definitions(IDQDevices PRIVATE MSGPACK_DISABLE_LEGACY_CONVERT=true)

    target_link_libraries(${PROJECT_NAME} PUBLIC
        CQPToolkit_Shared
        QKDInterfaces_Shared
        gRPC::grpc++
        protobuf::libprotobuf)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        IDQ4PClasses
        DomainModel
        Utilities
        PkgConfig::ZeroMQ
        PkgConfig::MsgPack)

    target_link_libraries(${PROJECT_NAME}_Shared PRIVATE
        IDQ4PClasses
        DomainModel
        Utilities
        PkgConfig::ZeroMQ
        PkgConfig::MsgPack)

endif()
