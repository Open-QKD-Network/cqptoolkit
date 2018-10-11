### @file
### @brief CQP Toolkit - CQP Toolkit
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

# Run some macro to setup some variables
include(CppSetup)
# build the gRPC interfaces
if(TARGET gRPC::grpc)
    add_subdirectory(QKDInterfaces)
endif(TARGET gRPC::grpc)

# Add the library build
add_subdirectory(CQPToolkit)
# Pull the value from the subfolder
get_directory_property(CQPToolkit_INCLUDE_DIRS DIRECTORY CQPToolkit DEFINITION CQPToolkit_INCLUDE_DIRS)
# Add windows specific projects
if(WIN32 AND OSIS64 AND NOT MINGW)
	add_subdirectory(DllArchBridge)
endif(WIN32 AND OSIS64 AND NOT MINGW)

# Add more projects/code
add_subdirectory(Tools)
add_subdirectory(UI)

if(BUILD_CQP_EXAMPLES)
	add_subdirectory(Examples)
endif(BUILD_CQP_EXAMPLES)

if(BUILD_TESTING AND GTEST_FOUND AND GMOCK_FOUND)
	add_subdirectory(tests)
endif(BUILD_TESTING AND GTEST_FOUND AND GMOCK_FOUND)

########### Output Setup ###########
message("")
message(STATUS "===== Configuration =====")
message(STATUS "System:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Version:         ${BUILD_VERSION}")
message(STATUS "BUILD_SHARED:    ${BUILD_SHARED}")
message(STATUS "BUILD_STATIC:    ${BUILD_STATIC}")
message(STATUS "Code Profiling:  ${CODE_PROF}")
message(STATUS "Build Tests:     ${BUILD_TESTING}")
message(STATUS "Shared Libs:     ${CMAKE_SHARED_LIBRARY_PREFIX}<name>${CMAKE_SHARED_LIBRARY_SUFFIX}")
message(STATUS "Static Libs:     ${CMAKE_STATIC_LIBRARY_PREFIX}<name>${CMAKE_STATIC_LIBRARY_SUFFIX}")
message(STATUS "=========================")
message("")
####################################
