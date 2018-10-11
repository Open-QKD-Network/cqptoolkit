# - Try to find gmock
# Once done this will define
#
#  GMOCK_FOUND - system has libusb
#  GMOCK_INCLUDE_DIRS - the libusb include directory
#  GMOCK_LIBRARIES - Link these to use libusb
#  GMOCK_DEFINITIONS - Compiler switches required for using libusb
#
#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libusb) Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

if (GMOCK_LIBRARIES AND GMOCK_INCLUDE_DIRS)
  # in cache already
  set(GMOCK_FOUND TRUE)
else (GMOCK_LIBRARIES AND GMOCK_INCLUDE_DIRS)
  
	# Use the package PkgConfig to detect headers/library files
        # pkgconfig for gmock doesn't include includes!
        #find_package(PkgConfig QUIET)
        #if(${PKG_CONFIG_FOUND})
        #	pkg_check_modules(GMOCK QUIET gmock)
        #endif(${PKG_CONFIG_FOUND})

endif(GMOCK_LIBRARIES AND GMOCK_INCLUDE_DIRS)

if(NOT GMOCK_FOUND)
  find_path(GMOCK_INCLUDE_DIRS
    NAMES
      gmock/gmock.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
  )

  if(CYGWIN)
      SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dll" ".dll.a" ".lib")
  endif(CYGWIN)

  find_library(GMOCK_LIBRARIES NAMES gmock gmockd
    PATH_SUFFIXES
      "lib/x86_64-linux-gnu/"
      "lib/${CMAKE_BUILD_TYPE}"
  )

  if (GMOCK_INCLUDE_DIRS AND GMOCK_LIBRARIES)
     set(GMOCK_FOUND TRUE)
  endif (GMOCK_INCLUDE_DIRS AND GMOCK_LIBRARIES)

  if (GMOCK_FOUND)
    add_library(gmock SHARED IMPORTED) # or STATIC instead of SHARED
    set_target_properties(gmock PROPERTIES
      IMPORTED_LOCATION "${GMOCK_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${GMOCK_INCLUDE_DIRS}"
    )

  else (GMOCK_FOUND)
    if (GMOCK_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find gmock")
    endif (GMOCK_FIND_REQUIRED)
  endif (GMOCK_FOUND)

  # show the GMOCK_INCLUDE_DIRS and GMOCK_LIBRARIES variables only in the advanced view
  mark_as_advanced(GMOCK_INCLUDE_DIRS GMOCK_LIBRARIES)

endif (NOT GMOCK_FOUND)
