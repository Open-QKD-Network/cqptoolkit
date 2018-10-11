### @file
### @brief CQP Toolkit - GTK3 setup
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 21 July 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
cmake_minimum_required (VERSION 3.7.2)

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig QUIET)
if(${PKG_CONFIG_FOUND})
	pkg_check_modules(GTKMM QUIET gtkmm-3.0)
	if(${GTKMM_FOUND})
		# Setup CMake to use GTK+, tell the compiler where to look for headers
		# and to the linker where to look for libraries
		include_directories(${GTKMM_INCLUDE_DIRS})
		link_directories(${GTKMM_LIBRARY_DIRS})
		# Add other flags to the compiler
		add_definitions(${GTKMM_CFLAGS_OTHER})
	endif()
endif()

macro (add_glade)

    #add extra non-standard files
    file(GLOB EXTRA_SOURCES LIST_DIRECTORIES false "*.glade")

    foreach(_glade ${EXTRA_SOURCES})
        list(APPEND ${PROJECT_NAME}_SOURCES ${_glade})
        get_filename_component(_glade_name ${_glade} NAME_WE)
        # TODO: make this automatic forr the glade file type
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_glade_name}.h
            COMMAND xsltproc --output ${CMAKE_CURRENT_BINARY_DIR}/${_glade_name}.h ${CMAKE_HOME_DIRECTORY}/GtkToCpp.xslt ${_glade}
            MAIN_DEPENDENCY ${_glade}
            DEPENDS ${_glade} ${CMAKE_HOME_DIRECTORY}/GtkToCpp.xslt
        )
        list(APPEND ${PROJECT_NAME}_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${_glade_name}.h)
    endforeach()

endmacro (add_glade)
