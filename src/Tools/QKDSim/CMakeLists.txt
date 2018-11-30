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

project(QKDSim CXX)
include(CommonSetup)

# projects below here will be put into this package
SET_COMPONENT_NAME("Simulate" CQP_INSTALL_COMPONENT)

# MAke standard program which uses the CQP Toolkit
CQP_EXE_PROJECT()

target_link_libraries(${PROJECT_NAME} PRIVATE
    CQPToolkit_Static
    QKDInterfaces_Static
    gRPC::grpc++
    protobuf::libprotobuf
    libCurl
    PkgConfig::CRYPTOPP
    CQPAlgorithms_Static
    )


if(TARGET OpenCL::OpenCL)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL::OpenCL)
endif()

if(TARGET PkgConfig::avahi-client)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::avahi-client)
endif()

if(TARGET PkgConfig::SQLITE3)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::SQLITE3)
endif()
