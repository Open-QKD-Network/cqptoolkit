### @file
### @brief CQP Toolkit - Tests - Drivers
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

Project (AlignmentTests C CXX)

# Run some macro to setup some variables
include(CommonSetup)
include(CppSetup)

# Generate a standard setup for a test
CQP_TEST_PROJECT()

target_link_libraries(${PROJECT_NAME}  PRIVATE
    CQPToolkit_Shared)
