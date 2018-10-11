# - Try to find libusb-1.0
# Once done this will define
#
#  LIBUSB_1_FOUND - system has libusb
#  LIBUSB_1_INCLUDE_DIRS - the libusb include directory
#  LIBUSB_1_LIBRARIES - Link these to use libusb
#  LIBUSB_1_DEFINITIONS - Compiler switches required for using libusb
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

# Use the package PkgConfig to detect headers/library files
find_package(PkgConfig QUIET)
if(${PKG_CONFIG_FOUND})
        pkg_check_modules(libUSB_1 QUIET IMPORTED_TARGET libusb-1.0)
endif(${PKG_CONFIG_FOUND})

if(NOT LIBUSB_1_FOUND)
  find_path(LIBUSB_1_INCLUDE_DIRS
    NAMES
      libusb-1.0/libusb.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
  )

  if(CYGWIN)
      SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dll" ".dll.a" ".lib")
  endif(CYGWIN)

  find_library(LIBUSB_1_LIBRARIES NAMES libusb libusb-1.0
    PATH_SUFFIXES
      "lib/x86_64-linux-gnu/"
      "lib/native/${MSVC_C_ARCHITECTURE_ID}"
  )

  if (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)
     set(LIBUSB_1_FOUND TRUE)
  endif (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)

  if (LIBUSB_1_FOUND)
    add_library(libusb SHARED IMPORTED) # or STATIC instead of SHARED
    set_target_properties(libusb PROPERTIES
      IMPORTED_LOCATION "${LIBUSB_1_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_1_INCLUDE_DIRS}"
    )

    if(WIN32)

      find_file( LIBUSB_1_DLLS
          NAMES libusb-1.0.dll
          PATHS ${CMAKE_LIBRARY_PATH} ${CMAKE_PROGRAM_PATH}
          PATH_SUFFIXES 
             "lib/native/${MSVC_C_ARCHITECTURE_ID}"
      )
      if(LIBUSB_1_DLLS)
        file(COPY ${LIBUSB_1_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
        LIST(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${LIBUSB_1_DLLS}")
      endif()
    endif(WIN32)

  else (LIBUSB_1_FOUND)
    if (libusb_1_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libusb")
    endif (libusb_1_FIND_REQUIRED)
  endif (LIBUSB_1_FOUND)

  # show the LIBUSB_1_INCLUDE_DIRS and LIBUSB_1_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBUSB_1_INCLUDE_DIRS LIBUSB_1_LIBRARIES)

endif (NOT LIBUSB_1_FOUND)
