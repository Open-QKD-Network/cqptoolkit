### @file
### @brief CQP Toolkit - Resource management macos
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 16 Aug 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
cmake_minimum_required (VERSION 3.7.2)
include(GTKMM)

if(GTKMM_LIBRARIES)

	### A cmake function for compiling arbritary blocks of data into an executable so that
	### they can be accessed by name using the glib resource compiler.
	### @param[in] xmlFile The xml file which defines all the resources to be compiled in to the program
	### @param[in,out] sourceList This list will be appended to with the names of the generated files so that they can be added to a project
	macro(AddResource xmlFile sourceList)
		get_filename_component(inputFilename ${xmlFile} NAME)
                string(REGEX REPLACE "\.xml$" ".cpp" outputFilename ${inputFilename})
		get_filename_component(workingDir ${xmlFile} DIRECTORY)

		execute_process(COMMAND glib-compile-resources --generate-dependencies ${inputFilename} 
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${workingDir}
                        OUTPUT_VARIABLE resourceDepsArgs)

                string(REGEX REPLACE "\n" ";" resourceDeps "${resourceDepsArgs}")
                PREPEND(resourceDeps "${workingDir}")

	    ADD_CUSTOM_COMMAND(
                OUTPUT
                   ${CMAKE_CURRENT_BINARY_DIR}/${outputFilename}
                   AnonExistentFile.missing # Fake file to make the target always build
	        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${workingDir}
	        COMMAND glib-compile-resources --internal --target=${CMAKE_CURRENT_BINARY_DIR}/${outputFilename} --generate-source ${inputFilename}
                MAIN_DEPENDENCY ${inputFilename}
	        DEPENDS ${resourceDeps} ${workingDir}/${inputFilename}
	    )

	    list(APPEND ${sourceList} ${CMAKE_CURRENT_BINARY_DIR}/${outputFilename})

	endmacro(AddResource xmlFile)

endif(GTKMM_LIBRARIES)
