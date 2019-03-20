### @file
### @brief CQP Toolkit - Tools - Handheld Alice
### 
### @copyright Copyright (C) University of Bristol 2019
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 06 Feb 2019
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
cmake_minimum_required (VERSION 3.7.2)

project(DummyQKDDriver CXX)
include(CommonSetup)

# add the proto files from the QKDInterfaces project
get_target_property(QKDInterfaces_SOURCE_DIR QKDInterfaces SOURCE_DIR)
LIST(APPEND PROTOBUF_IMPORT_DIRS ${QKDInterfaces_SOURCE_DIR})
ADD_GRPC_FILES()

# Make standard program which uses the CQP Toolkit
CQP_EXE_PROJECT()

# for static linking
#target_link_libraries(${PROJECT_NAME} PRIVATE
#    KeyManagement_Static
#    CQPToolkit_Static
#    QKDInterfaces_Static
#    Algorithms_Static
#    PkgConfig::libUSB_1
#    gRPC::grpc++
#    protobuf::libprotobuf
#    )

get_target_property(QKDInterfaces_BINARY_DIR QKDInterfaces BINARY_DIR)
target_include_directories(${PROJECT_NAME} PRIVATE "${QKDInterfaces_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE
    KeyManagement_Shared
    CQPToolkit_Shared
    )
