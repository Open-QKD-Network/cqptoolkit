### @file
### @brief CQP Toolkit - Tools - Alice
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
# Define a project to add to the build
project(IDQKeyExtraction CXX)
# Setup some standard variables based on the build system
include(CommonSetup)

# MAke standard program which uses the CQP Toolkit
CQP_EXE_PROJECT()
target_link_libraries(${PROJECT_NAME}  PRIVATE
    CQPToolkit_Shared)
