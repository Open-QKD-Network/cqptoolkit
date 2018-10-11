### @file
### @brief CQP Toolkit
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

# Specify how we expect cmake to behave
cmake_policy(VERSION 3.5)

if(MSVC)
	message(STATUS "Finding Nuget Packages")

  FILE(GLOB_RECURSE _children LIST_DIRECTORIES true "${CMAKE_BINARY_DIR}/Packages/*")
  FOREACH(_child ${_children})
    IF(IS_DIRECTORY "${_child}")
      #message("   Looking at: ${_child}")
      get_filename_component(_childName "${_child}" NAME)
      get_filename_component(_childParent "${_child}" DIRECTORY)
      if(${_childName} STREQUAL "include")
      	#message("      Found : ${_child}")
      	LIST(APPEND CMAKE_PREFIX_PATH "${_childParent}")
      
      elseif(${_childName} STREQUAL "lib")
      	#message("      Found : ${_child}")
      	LIST(APPEND CMAKE_LIBRARY_PATH "${_childParent}")
      
      elseif(${_childName} STREQUAL "bin")
      	#message("      Found : ${_child}")
      	LIST(APPEND CMAKE_PROGRAM_PATH "${_childParent}")
      
      elseif(${_childName} STREQUAL "tools")
      	#message("      Found : ${_child}")
      	LIST(APPEND CMAKE_PROGRAM_PATH "${_child}")
      
      elseif(${_childName} STREQUAL "cmake")
      	#message("      Found : ${_child}")
         LIST(APPEND CMAKE_PREFIX_PATH "${_childParent}")
      endif()  
  ENDIF()
  ENDFOREACH()
  SET(_children)
  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endif(MSVC)