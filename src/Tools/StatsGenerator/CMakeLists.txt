### @file
### @brief CQP Toolkit - Tools - Bob
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
# See: https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.7.2)

project(StatsGenerator CXX)
include(CommonSetup)

# projects below here will be put into this package
SET_COMPONENT_NAME("Simulate" CQP_INSTALL_COMPONENT)

# MAke standard program which uses the CQP Toolkit
CQP_EXE_PROJECT()

target_link_libraries(${PROJECT_NAME} PRIVATE
    CQPToolkit_Static
    QKDInterfaces_Static
    KeyManagement_Static
    Algorithms_Static
    PkgConfig::libUSB_1
    libCurl
    gRPC::grpc++
    protobuf::libprotobuf
    )

if(TARGET PkgConfig::avahi-client)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        PkgConfig::avahi-client)
endif()
# add the interface files for installation
#INSTALL(FILES
#    "${CMAKE_SOURCE_DIR}/src/QKDInterfaces/Reporting.proto"
#    "${CMAKE_SOURCE_DIR}/src/QKDInterfaces/IReporting.proto"
#    DESTINATION "include/QKDInterfaces"
#    COMPONENT "${CQP_INSTALL_COMPONENT}")
