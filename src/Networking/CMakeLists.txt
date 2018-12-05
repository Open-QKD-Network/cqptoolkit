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

# Maps to Visual Studio solution file (Tutorial.sln)
# The solution will have all targets (exe, lib, dll)
# as Visual Studio projects (.vcproj)
project (Networking C CXX)

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

# Version Information
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)
set(${PROJECT_NAME}_VERSION_PATCH 0)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH})

# Generate a standard setup for a library
CQP_LIBRARY_PROJECT()
# Now tell the compiler and linker where to find specific libs

add_dependencies(${PROJECT_NAME} KeyManagement)
add_dependencies(${PROJECT_NAME}_Shared KeyManagement_Shared)
add_dependencies(${PROJECT_NAME}_Static KeyManagement_Static)

if(TARGET ${PROJECT_NAME}_Shared)

    target_link_libraries(${PROJECT_NAME}_Shared PUBLIC
        KeyManagement_Shared
        libCurl
    )

endif(TARGET ${PROJECT_NAME}_Shared)
# packaging
SET(CPACK_COMPONENT_${PROJECT_NAME}_DESCRIPTION   "CQP Networking")
