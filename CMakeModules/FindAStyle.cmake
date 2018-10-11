#.rst:
# FindAStyle
# -----------
#
# This module looks for AStyle and the path to Graphviz's dot
#
# AStyle is a documentation generation tool.  Please see
# http://www.astyle.org
#
# This module accepts the following optional variables:
#
# ::
#
#    ASTYLE_SKIP_DOT       = If true this module will skip trying to find Dot
#                             (an optional component often used by AStyle)
#
#
#
# This modules defines the following variables:
#
# ::
#
#    ASTYLE_EXECUTABLE     = The path to the astyle command.
#    ASTYLE_FOUND          = Was AStyle found or not?
#    ASTYLE_VERSION        = The version reported by astyle --version
#
#
#
# ::
#
#    ASTYLE_DOT_EXECUTABLE = The path to the dot program used by astyle.
#    ASTYLE_DOT_FOUND      = Was Dot found or not?
#
# For compatibility with older versions of CMake, the now-deprecated
# variable ``ASTYLE_DOT_PATH`` is set to the path to the directory
# containing ``dot`` as reported in ``ASTYLE_DOT_EXECUTABLE``.
# The path may have forward slashes even on Windows and is not
# suitable for direct substitution into a ``Doxyfile.in`` template.
# If you need this value, use :command:`get_filename_component`
# to compute it from ``ASTYLE_DOT_EXECUTABLE`` directly, and
# perhaps the :command:`file(TO_NATIVE_PATH)` command to prepare
# the path for a AStyle configuration file.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# For backwards compatibility support
if(AStyle_FIND_QUIETLY)
  set(ASTYLE_FIND_QUIETLY TRUE)
endif()

#
# Find AStyle...
#

find_program(ASTYLE_EXECUTABLE
  NAMES astyle AStyle.exe
  PATHS
    /Applications/AStyle.app/Contents/Resources
    /Applications/AStyle.app/Contents/MacOS
  DOC "AStyle code prety printer (http://astyle.sourceforge.net/)"
)

if(ASTYLE_EXECUTABLE)
  execute_process(COMMAND ${ASTYLE_EXECUTABLE} "--version" OUTPUT_VARIABLE ASTYLE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AStyle REQUIRED_VARS ASTYLE_EXECUTABLE VERSION_VAR ASTYLE_VERSION)

if(APPLE)
  # Restore the old app-bundle setting setting
  set(CMAKE_FIND_APPBUNDLE ${TEMP_ASTYLE_SAVE_CMAKE_FIND_APPBUNDLE})
endif()

# For backwards compatibility support
set (ASTYLE ${ASTYLE_EXECUTABLE} )

mark_as_advanced(
  ASTYLE_EXECUTABLE
  ASTYLE_DOT_EXECUTABLE
  )
