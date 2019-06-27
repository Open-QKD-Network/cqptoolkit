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
cmake_minimum_required (VERSION 3.7.2)

project(IDQDevices CXX)

# projects below here will be put into this package
SET_COMPONENT_NAME("Drivers" CQP_INSTALL_COMPONENT)

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

ADD_GRPC_FILES()

CQP_LIBRARY_PROJECT()

if(TARGET IDQ4PClasses)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DHAVE_IDQ4P)

    target_compile_definitions(IDQDevices PRIVATE MSGPACK_DEFAULT_API_VERSION=1)
    target_compile_definitions(IDQDevices PRIVATE IDQ4P_DEBUG=true)
    target_compile_definitions(IDQDevices PRIVATE IDQ4P_ENABLE_SERIALIZER=true)

    #target_compile_definitions(IDQDevices PRIVATE MSGPACK_USE_DEFINE_MAP=true)
    #target_compile_definitions(IDQDevices PRIVATE MSGPACK_DISABLE_LEGACY_CONVERT=true)

endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.12.0)

    target_link_libraries(${PROJECT_NAME} PUBLIC
        CQPToolkit_Shared
        QKDInterfaces_Shared
        gRPC::grpc++
        protobuf::libprotobuf)

    if(TARGET IDQ4PClasses)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            IDQ4PClasses
            IDQ4PDomainModel
            IDQ4PUtilities
            PkgConfig::ZeroMQ
            PkgConfig::MsgPack)
    endif()
elseif(TARGET IDQ4PClasses)
    add_dependencies(${PROJECT_NAME} PkgConfig::ZeroMQ PkgConfig::MsgPack)
    get_target_property(IDQ4PCLASSES_SOURCE_DIR IDQ4PClasses INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(IDQ4PDOMAINMODEL_SOURCE_DIR IDQ4PDomainModel INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(IDQ4PUTILITIES_SOURCE_DIR IDQ4PUtilities INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(MSGPACK_SOURCE_DIR PkgConfig::MsgPack INTERFACE_INCLUDE_DIRECTORIES)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${IDQ4PCLASSES_SOURCE_DIR}
        ${IDQ4PDOMAINMODEL_SOURCE_DIR}
        ${IDQ4PUTILITIES_SOURCE_DIR}
        ${MSGPACK_SOURCE_DIR})
endif()

if(TARGET ${PROJECT_NAME}_Shared AND TARGET IDQ4PClasses)
    target_link_libraries(${PROJECT_NAME}_Shared PRIVATE
        IDQ4PClasses
        IDQ4PDomainModel
        IDQ4PUtilities
        PkgConfig::ZeroMQ
        PkgConfig::MsgPack)

endif()
# packaging
SET(CPACK_COMPONENT_${PROJECT_NAME}_DESCRIPTION   "IDQuantique library")
