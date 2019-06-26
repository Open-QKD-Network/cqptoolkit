### @file
### @brief CQP Toolkit - Tests
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

project(CQPTests C CXX)

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

if(MINGW)
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
endif(MINGW)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include_directories(${CQPToolkit_INCLUDE_DIRS})
#add_definitions("-DGTEST_LINKED_AS_SHARED_LIBRARY=1")

# Each suite of tests is built into an executable

# projects below here will be put into this package
SET_COMPONENT_NAME("exclude" CQP_INSTALL_COMPONENT)

ADD_ALL_SUBDIRS()

