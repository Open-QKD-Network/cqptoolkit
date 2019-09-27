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

project(FreespaceTest CXX)
include(CommonSetup)

# MAke standard program which uses the CQP Toolkit
CQP_EXE_PROJECT()

# for static linking
#target_link_libraries(${PROJECT_NAME} PRIVATE
#    KeyManagement_Static
#    CQPToolkit_Static
#    QKDInterfaces_Static
#    Algorithms_Static
#    libusb_1
#    gRPC::grpc++
#    protobuf::libprotobuf
#    )

target_link_libraries(${PROJECT_NAME} PRIVATE
    CQPToolkit_Shared
    )
