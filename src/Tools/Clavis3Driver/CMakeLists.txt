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

if(TARGET IDQ_IDQ4PClasses)
    project(Clavis3Driver CXX)
    include(CommonSetup)

    # Make standard program which uses the CQP Toolkit
    CQP_EXE_PROJECT()

    if(TARGET Clavis3Driver)

        target_compile_definitions(Clavis3Driver PRIVATE MSGPACK_DEFAULT_API_VERSION=1)
        #target_compile_definitions(Clavis3Driver PRIVATE IDQ4P_ENABLE_SERIALIZER=true)
        target_compile_definitions(Clavis3Driver PRIVATE IDQ4P_DEBUG=true)
        #target_compile_definitions(Clavis3Driver PRIVATE MSGPACK_USE_DEFINE_MAP=true)
        #target_compile_definitions(Clavis3Driver PRIVATE MSGPACK_DISABLE_LEGACY_CONVERT=true)

        target_link_libraries(${PROJECT_NAME} PRIVATE
            CQPToolkit_Shared
            QKDInterfaces_Shared
            PkgConfig::ZeroMQ
            PkgConfig::MsgPack
            IDQ_IDQ4PClasses
            IDQ_DomainModel
            IDQ_Utilities
            )
    endif()
endif()
