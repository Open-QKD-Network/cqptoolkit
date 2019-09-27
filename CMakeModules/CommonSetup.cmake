### @file
### @brief CQP Toolkit - Standard setup macro
###
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
###
### Setup CMake

# See: https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.7.2)

set(verbose FALSE)

macro(include_once)
  if (INCLUDED_${CMAKE_CURRENT_LIST_FILE})
    return()
  endif()
  set(INCLUDED_${CMAKE_CURRENT_LIST_FILE} true)
endmacro()

include_once()

# Enable ExternalProject CMake module
include(ExternalProject REQUIRED)

# add a cmake feature for dll exports
include (GenerateExportHeader)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Dont llow build files to be placed in the original source folder (src)
# This keeps the code tree uncluttered
set(CMAKE_DISABLE_IN_SOURCE_BUILD   ON)
# Allow the buld to change files during the build process, eg generate files.
set(CMAKE_DISABLE_SOURCE_CHANGES    ON)
# Set this to On to see exactly what make is doing
set(CMAKE_VERBOSE_MAKEFILE          OFF)
set(CMAKE_COLOR_MAKEFILE            ON)
SET(CMAKE_USE_RELATIVE_PATHS        ON)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR       ON)
set(FIND_LIBRARY_USE_LIB64_PATHS    ON)

if(WIN32)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
endif(WIN32)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()

# ============= Global Properties ==========
if((NOT CMAKE_BUILD_TYPE) OR ("${CMAKE_BUILD_TYPE}" STREQUAL ""))
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel.")
endif()

if(WIN32 AND "$ENV{PROCESSOR_ARCHITECTURE}" MATCHES "64")
    message(STATUS "Setting default target system to 64bit")
    
    set(ENV{VSCMD_ARG_HOST_ARCH} x64)
    set(ENV{VSCMD_ARG_TGT_ARCH} x64)
endif()

# Turn on the C language to detect the target word length
enable_language(C)

if(MSVC AND "$ENV{VSCMD_ARG_TGT_ARCH}" MATCHES "64")
    set(MSVC_C_ARCHITECTURE_ID x64)
endif(MSVC AND "$ENV{VSCMD_ARG_TGT_ARCH}" MATCHES "64")

### @def LIST_ALL_VARIABLES
### Dump all cmake variables and their values
### Useful for debugging
MACRO(LIST_ALL_VARIABLES)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
ENDMACRO(LIST_ALL_VARIABLES)

### @def PREPEND
### Add a path <prefix> to the front of every value passed after the prefix
### @detail
### PREPEND out a one two => out = "a/one;a/two"
### @param var The name of the output variable where the result is stored
### @param prefix the path name to add to each subsequent value
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

### @def SUBDIRLIST
### Build a list of directories under the path
### @param result output variable name for the result
### @param curdir Starting path to search
MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO(SUBDIRLIST)

### @def ADD_ALL_SUBDIRS
### Call ADD_SUBDIRECTORY for every path containing a CMakeLists.txt file
### below the current source directory
MACRO(ADD_ALL_SUBDIRS)
    SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})
    FOREACH(subdir ${SUBDIRS})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt")
            ADD_SUBDIRECTORY(${subdir})
        endif()
    ENDFOREACH()
endMACRO(ADD_ALL_SUBDIRS)

### @fn DOWNLOAD
### Download a file
### @param DOWNLOAD_URL url to download
### @param DOWNLOAD_FILE output filename
### @param DOWNLOAD_MD5 optional md5sum value to verify download
function(DOWNLOAD DOWNLOAD_URL DOWNLOAD_FILE DOWNLOAD_MD5)

    if(NOT DOWNLOAD_MD5 STREQUAL "")
        set(_opts EXPECTED_MD5 ${DOWNLOAD_MD5})
    else(NOT DOWNLOAD_MD5 STREQUAL "")
        set(_opts "")
    endif(NOT DOWNLOAD_MD5 STREQUAL "")

    file(DOWNLOAD "${DOWNLOAD_URL}" "${DOWNLOAD_FILE}" 
        ${_opts}
        SHOW_PROGRESS 
        TIMEOUT 30
        INACTIVITY_TIMEOUT 30
        STATUS _downloadStatus)

    LIST(GET _downloadStatus 0 _downloadExitCode)
    if(NOT _downloadExitCode EQUAL 0)
        message(FATAL_ERROR "Failed to download ${DOWNLOAD_FILE}: ${_downloadStatus}. Removing stale file")
        file(REMOVE "${DOWNLOAD_FILE}")
    endif()
endfunction(DOWNLOAD)

### @def SET_COMPONENT_NAME
### Finalise the component name based on system settings
### @param BASE project name to finalise
### @param OUT_VAR destination for final name
macro(SET_COMPONENT_NAME BASE OUT_VAR)
    SET(${OUT_VAR} "${BASE}")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        STRING(APPEND ${OUT_VAR} "D")
    endif()
endmacro(SET_COMPONENT_NAME)

### Call this to embed raw binary files into a project
### The result will be added to the list of project sources.
### Needs to be called before any project/add_executable/etc call.
### @details
### The final executable will contain 3 extra symbols:
### _binary_<filename>_start - The location of the start of the data
### _binary_<filename>_end - The location of the end of the data
### _binary_<filename>_start - The number of bytes
### dots in the filename are replaced by '_', use the symbols with:
### @code
###   extern char _binary_Histogram_cl_start[];
###   extern size_t _binary_Histogram_cl_size;
### @endcode
### @note
###    Modifies: ${PROJECT_NAME}_OBJS
### @param filenames a list of files to wrap in the executable format
MACRO (EmbedResources filenames)

    foreach(filename ${filenames})
        get_filename_component(fileprefix ${filename} NAME)
        get_filename_component(filepath ${filename} PATH)
        set(fileOut "${CMAKE_CURRENT_BINARY_DIR}/${fileprefix}.o")

        message("Adding resource ${fileprefix}")
        # convert the file to elf, then change the section to readonly
        ADD_CUSTOM_COMMAND(
            OUTPUT ${fileOut}
            MAIN_DEPENDENCY ${filename}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${filepath}
            COMMAND ld
            ARGS -r -b binary -o ${fileOut} ${fileprefix}
            COMMAND ${CMAKE_OBJCOPY}
            ARGS --rename-section ".data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA" ${fileOut} ${fileOut}
            )

        string(REPLACE "." "_" symbolName "${fileprefix}")

        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${fileprefix}.h"
            "// symbols generated from resouce file ${fileprefix}\n"
            "extern \"C\" {\n"
            "   extern const char _binary_${symbolName}_start[];\n"
            "   extern const char _binary_${symbolName}_end;\n"
            "   extern const size_t _binary_${symbolName}_size;\n"
            "   const size_t _binary_${symbolName}_realsize = &_binary_${symbolName}_end - &_binary_${symbolName}_start[0];\n"
            "}\n")
        LIST(APPEND ${PROJECT_NAME}_OBJS ${fileOut})
        SET_SOURCE_FILES_PROPERTIES(
          ${fileOut}
          PROPERTIES
          EXTERNAL_OBJECT true
          GENERATED true
          LINKER_LANGUAGE C
        )
    endforeach(filename)
ENDMACRO (EmbedResources filename)

### @def CQP_PROJECT
### Creates a project, performing common steps used in CQP
### Place after declaring a project and and non-standard additions to PROJECT_SOURCE_DIR
macro(CQP_PROJECT)

  # Version Information
  set(${PROJECT_NAME}_VERSION_MAJOR ${BUILD_VERSION_MAJOR})
  set(${PROJECT_NAME}_VERSION_MINOR ${BUILD_VERSION_MINOR})
  set(${PROJECT_NAME}_VERSION_PATCH ${BUILD_VERSION_PATCH})
  set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH})

  # search through specific folders for files to build
  file(GLOB_RECURSE tempSources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.h" "*.hpp")
  list(APPEND ${PROJECT_NAME}_HEADERS ${tempSources})
  file(GLOB_RECURSE tempSources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.c" "*.cpp")
  list(APPEND ${PROJECT_NAME}_SOURCES ${tempSources} ${${PROJECT_NAME}_HEADERS})

  # include the version file
  list(APPEND ${PROJECT_NAME}_SOURCES "${CMAKE_BINARY_DIR}/src/Version.cpp")
  # make gcc behave like MS build, classes should be explicitly exported
  # use class CQPTOOLKIT_EXPORT <name> to export
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

  SET(CPACK_DEBIAN_${PROJECT_NAME}_PACKAGE_NAME "${PROJECT_NAME}")

  SET(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wextra -pedantic -Wno-unused-parameter")
endmacro(CQP_PROJECT)

### @def CQP_LIBRARY_PROJECT
### Creates a library project, performing common steps used in CQP
### Place after declaring a project and and non-standard additions to PROJECT_SOURCE_DIR
### Add object file to include directly by adding them to the ${PROJECT_NAME}_OBJS list
macro(CQP_LIBRARY_PROJECT)
    # Set Properties->General->Configuration Type to Application(.exe)
    # Creates qkdTest.exe with the listed sources (main.cxx)
    # Adds sources to the Solution Explorer

    # With this system, the build environment (msbuild/gcc) does not know when you add/remove
    # files from the build so you will need to ether rebuild or touch a CMakeLists.txt file to
    # force cmake to scan for files.

    CQP_PROJECT()
    # Create an object library which can later be turned into a static and/or dynamic library
    # see: https://cmake.org/Wiki/CMake/Tutorials/Object_Library
    # This removes the need to compile the code twice to produce both libraries
    add_library (${PROJECT_NAME} OBJECT ${${PROJECT_NAME}_SOURCES})

    # specify the include folders - this will help with dependecies later on.
    target_include_directories(${PROJECT_NAME}
        #PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        #PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        PUBLIC $<INSTALL_INTERFACE:include>)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER "${${PROJECT_NAME}_HEADERS}")

    # for dependencies with other projects
    include_directories("${CMAKE_BINARY_DIR}/src")

    # Produce a header file which defines whether functions are imported/exported.
    # Rename the macros so they match the original project name
    GENERATE_EXPORT_HEADER(${PROJECT_NAME})
    # Add the generated files and the header files to the output
    INSTALL(FILES
        # Including the following line will copy all the header files to the output
        # but without the folder structure
        #${${PROJECT_NAME}_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/$<LOWER_CASE:${PROJECT_NAME}>_export.h
        DESTINATION include/${PROJECT_NAME}
        COMPONENT ${CQP_INSTALL_COMPONENT}-dev)

    if(BUILD_SHARED)
        LIST(APPEND BUILD_TYPES "Shared")
    endif(BUILD_SHARED)

    if(BUILD_STATIC)
        LIST(APPEND BUILD_TYPES "Static")
    endif(BUILD_STATIC)

    foreach(_build_type ${BUILD_TYPES})
        STRING(TOUPPER ${_build_type} _build_type_upper)
        # This tells the builder to create a library (.lib/.a/etc)
        SET(PROJECT_NAME_${_build_type_upper} ${PROJECT_NAME}_${_build_type})

        # We're making two, one dynamic (shared), one static
        # Use the previously compiled object library for the code
        add_library (${PROJECT_NAME}_${_build_type} ${_build_type_upper} $<TARGET_OBJECTS:${PROJECT_NAME}> ${${PROJECT_NAME}_OBJS})
        # specify the include folders - this will help with dependecies later on.
        target_include_directories(${PROJECT_NAME}_${_build_type}
            #PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
            #PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
            PUBLIC $<INSTALL_INTERFACE:include>)

        SET(_libFileName "${PREFIX}${PROJECT_NAME}${LIBSUFFIX}")
        set_target_properties(${PROJECT_NAME}_${_build_type} PROPERTIES OUTPUT_NAME "${_libFileName}")

        # This stops the object library being built with dllimport symbols
        # Add some platform specific libraries to both shared and static links
        if(WIN32)
            target_link_libraries(${PROJECT_NAME}_${_build_type} PRIVATE "shlwapi" "setupapi" "Ws2_32.lib")
        elseif(UNIX)
            target_link_libraries(${PROJECT_NAME}_${_build_type} PRIVATE "pthread" "uuid")
        endif(WIN32)

        # This makes the project importable from the build directory
        export(TARGETS ${PROJECT_NAME}_${_build_type} FILE ${PROJECT_NAME}_${_build_type}Config.cmake)

        # Add it to the list of components the user can select
        list(APPEND CPACK_COMPONENTS_ALL ${PROJECT_NAME}_${_build_type})

        if(${PROJECT_NAME}_VERSION)
            set_target_properties(${PROJECT_NAME}_${_build_type} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION})
        endif()

        if(MSVC)
            set_property(TARGET ${PROJECT_NAME}_${_build_type} PROPERTY FOLDER "Libraries")
        endif(MSVC)
    endforeach(_build_type ${BUILD_TYPES})

    if(BUILD_SHARED)
        set_property(TARGET ${PROJECT_NAME}_Shared PROPERTY POSITION_INDEPENDENT_CODE ON)
        SET(CPACK_COMPONENT_${CQP_INSTALL_COMPONENT}_GROUP "Runtime")

        # Add this library to the list of things to install
        install(TARGETS ${PROJECT_NAME}_Shared COMPONENT ${CQP_INSTALL_COMPONENT}
            EXPORT ${PROJECT_NAME}
            RUNTIME DESTINATION bin COMPONENT ${CQP_INSTALL_COMPONENT}
            ARCHIVE DESTINATION lib COMPONENT ${CQP_INSTALL_COMPONENT}-dev
            LIBRARY DESTINATION lib COMPONENT ${CQP_INSTALL_COMPONENT}
            PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT ${CQP_INSTALL_COMPONENT}-dev
            PRIVATE_HEADER DESTINATION include/private/${PROJECT_NAME} COMPONENT ${CQP_INSTALL_COMPONENT}$-dev
        )

        # This makes the project importable from the install directory
        # Put config file in per-project dir (name MUST match), can also
        # just go into 'cmake'.
        #install(EXPORT ${PROJECT_NAME}_Shared DESTINATION lib/${PROJECT_NAME}_Shared  COMPONENT ${PROJECT_NAME})
    endif(BUILD_SHARED)

    if(BUILD_STATIC)
        SET(CPACK_COMPONENT_${CQP_INSTALL_COMPONENT}-dev_GROUP "Development")

        # Add this library to the list of things to install
        install(TARGETS ${PROJECT_NAME}_Static COMPONENT ${CQP_INSTALL_COMPONENT}-dev
            #EXPORT ${PROJECT_NAME}
            RUNTIME DESTINATION bin COMPONENT ${CQP_INSTALL_COMPONENT}
            ARCHIVE DESTINATION lib COMPONENT ${CQP_INSTALL_COMPONENT}-dev
            LIBRARY DESTINATION lib COMPONENT ${CQP_INSTALL_COMPONENT}
            PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT ${CQP_INSTALL_COMPONENT}-dev
            PRIVATE_HEADER DESTINATION include/private/${PROJECT_NAME} COMPONENT ${CQP_INSTALL_COMPONENT}-dev
        )

        # This makes the project importable from the install directory
        # Put config file in per-project dir (name MUST match), can also
        # just go into 'cmake'.
        #install(EXPORT ${PROJECT_NAME}_Static DESTINATION lib/${PROJECT_NAME}_Static  COMPONENT ${PROJECT_NAME}-dev)

    endif(BUILD_STATIC)

    # include the header files in the output structure
    # TODO: this seems clunky
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMPONENT ${CQP_INSTALL_COMPONENT}-dev
        DESTINATION include FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp")

    SET(CPACK_DEBIAN_PACKAGE_${CQP_INSTALL_COMPONENT}_SECTION "libs")
    SET(CPACK_DEBIAN_PACKAGE_${CQP_INSTALL_COMPONENT}-dev_SECTION "libs")
endmacro(CQP_LIBRARY_PROJECT)

### @def CQP_EXE_PROJECT
### Creates an exe project, performing common steps used in CQP
### Place after declaring a project and and non-standard additions to PROJECT_SOURCE_DIR
### Add object file to include directly by adding them to the ${PROJECT_NAME}_OBJS list
macro(CQP_EXE_PROJECT)

    CQP_PROJECT()
    include_directories("${CMAKE_BINARY_DIR}/src")

    # The project will build an executable with the source files found above
    # it will have the same name as the current project.
    add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_OBJS})

    if(${CMAKE_VERSION} VERSION_GREATER "3.8")
        target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
    endif()

    if(MSVC)
        set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Programs")
    endif()

    # Add this library to the list of things to install
    INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION bin COMPONENT ${CQP_INSTALL_COMPONENT}
    )

    SET(CPACK_DEBIAN_${CQP_INSTALL_COMPONENT}_PACKAGE_SECTION "bin")
endmacro(CQP_EXE_PROJECT)

### @def CQP_TEST_PROJECT special kind of CQP_EXE_PROJECT for tests
macro(CQP_TEST_PROJECT)
    CQP_EXE_PROJECT()

    include_directories("${GTEST_INCLUDE_DIRS}" "${GMOCK_INCLUDE_DIRS}")

    if(TARGET GTest_External)
        add_dependencies(${PROJECT_NAME} GTest_External)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE gtest gtest_main gmock)
    set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Tests")

    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

    if(MSVC)
        #TODO Copy
        set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Tests")
        set_property(TARGET ${PROJECT_NAME} PROPERTY EXCLUDE_FROM_DEFAULT_BUILD 1)   
    endif()

    #SET(CPACK_COMPONENT_${PROJECT_NAME}_GROUP "Tests")
    SET(CPACK_DEBIAN_${CQP_INSTALL_COMPONENT}_PACKAGE_SECTION "test")

endmacro(CQP_TEST_PROJECT)

### @def CQP_QT_PROJECT special kind of CQP_EXE_PROJECT for QT applications
### ui and qrc files are handled automatically
macro(CQP_QT_PROJECT)
    # Find the QtWidgets library
    find_package(Qt5 COMPONENTS Core Widgets QUIET)
    if(${Qt5Widgets_FOUND})
        # Instruct CMake to run moc automatically when needed.
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
        set(CMAKE_AUTOURCC ON)

        file(GLOB_RECURSE tempSources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.ui")
        file(GLOB_RECURSE tempResources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.qrc")
        list(APPEND ${PROJECT_NAME}_SOURCES ${tempSources} ${tempResources})

        QT5_ADD_RESOURCES(RESOURCES ${tempResources})
        list(APPEND ${PROJECT_NAME}_SOURCES ${RESOURCES})

        add_definitions(-DQT_DEPRECATED_WARNINGS)

        CQP_EXE_PROJECT()

        # Use the Widgets module from Qt 5.
        target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)

        if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            # create a desktop file for the program
            configure_file("${CMAKE_SOURCE_DIR}/packaging/app.desktop" "${PROJECT_NAME}.desktop")
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
                DESTINATION share/applications
                COMPONENT ${CQP_INSTALL_COMPONENT})
            install(FILES "${CMAKE_SOURCE_DIR}/packaging/qetlabs.png"
                DESTINATION share/icons
                RENAME "${PROJECT_NAME}.png"
                COMPONENT ${CQP_INSTALL_COMPONENT})
        endif()

    else(${Qt5Widgets_FOUND})
        message(WARNING "${PROJECT_NAME} skipped.")
    endif(${Qt5Widgets_FOUND})
endmacro(CQP_QT_PROJECT)

### @def CQP_QT_LIB_PROJECT special kind of CQP_EXE_PROJECT for QT libraries
### ui and qrc files are handled automatically
macro(CQP_QT_LIB_PROJECT)
    # Find the QtWidgets library
    find_package(Qt5 COMPONENTS Core Widgets QUIET)
    if(${Qt5Widgets_FOUND})
        # Instruct CMake to run moc automatically when needed.
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
        set(CMAKE_AUTOURCC ON)

        file(GLOB_RECURSE tempSources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.ui")
        file(GLOB_RECURSE tempResources LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.qrc")
        list(APPEND ${PROJECT_NAME}_SOURCES ${tempSources} ${tempResources})

        QT5_ADD_RESOURCES(RESOURCES ${tempResources})
        #list(APPEND ${PROJECT_NAME}_SOURCES ${RESOURCES})

        add_definitions(-DQT_DEPRECATED_WARNINGS)

        CQP_LIBRARY_PROJECT()

        # Use the Widgets module from Qt 5.
        if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
            # workaround for older cmake
            target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include/x86_64-linux-gnu/qt5/")
            target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include/x86_64-linux-gnu/qt5/QtWidgets")
        else()
            target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Widgets)
        endif()
        target_link_libraries(${PROJECT_NAME}_Shared PUBLIC Qt5::Widgets)
    else(${Qt5Widgets_FOUND})
        message(WARNING "${PROJECT_NAME} skipped.")
    endif(${Qt5Widgets_FOUND})
endmacro(CQP_QT_LIB_PROJECT)

macro(ADD_GRPC_FILES)
    if(TARGET protobuf::libprotobuf)
        # search through specific folders for files to build
        file(GLOB_RECURSE ${PROJECT_NAME}_INTERFACES LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.proto")

        get_target_property(Protobuf_IMPORT_DIRS protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
            set(PROTOBUF_GENERATE_CPP_APPEND_PATH)
            LIST(APPEND Protobuf_IMPORT_DIRS ${PROTOBUF_IMPORT_DIRS})
        # correct for messed up out dir
        LIST(GET ${PROJECT_NAME}_INTERFACES 0 _file)
        get_filename_component(_rel_dir ${_file} DIRECTORY)
        protobuf_generate(
            APPEND_PATH
            LANGUAGE cpp
            IMPORT_DIRS ${Protobuf_IMPORT_DIRS}
            OUT_VAR ${PROJECT_NAME}_PROTO_SRCS
            PROTOS ${${PROJECT_NAME}_INTERFACES})

        grpc_generate(
            APPEND_PATH
            LANGUAGE cpp
            IMPORT_DIRS ${Protobuf_IMPORT_DIRS}
            OUT_VAR ${PROJECT_NAME}_GRPC_SRCS
            PROTOS ${${PROJECT_NAME}_INTERFACES})

        include_directories(${Protobuf_IMPORT_DIRS})

        LIST(APPEND ${PROJECT_NAME}_SOURCES
            ${${PROJECT_NAME}_PROTO_SRCS}
            ${${PROJECT_NAME}_GRPC_SRCS})
    endif()
endmacro(ADD_GRPC_FILES)

### @def GRPC_PROJECT special kind of CQP_LIBRARY_PROJECT for generating gRPC interfaces
### proto files are handled automatically
macro(GRPC_PROJECT)

    ADD_GRPC_FILES()

    CQP_LIBRARY_PROJECT()

    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET default)
    set_target_properties(${PROJECT_NAME}_Shared PROPERTIES CXX_VISIBILITY_PRESET default)

    add_dependencies(${PROJECT_NAME} gRPC::grpc++ protobuf::libprotobuf)

    # add the interface files for installation
    INSTALL(FILES ${${PROJECT_NAME}_INTERFACES} ${${PROJECT_NAME}_PROTO_HDRS} ${${PROJECT_NAME}_GRPC_HDRS}
        DESTINATION "include/${PROJECT_NAME}/"
        COMPONENT "${CQP_INSTALL_COMPONENT}-dev")

endmacro(GRPC_PROJECT)
