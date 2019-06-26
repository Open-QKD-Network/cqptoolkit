### @file
### @brief CQP Toolkit - CQP Top level project
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
# Specify some settings which will be stored in the cache,
# these can be changed on the command line by adding -D<name>=<value>
# eg -DBUILD_TESTING=OFF
# Changing them in this file wont changed any values already stored in the cache

# See: https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.7.2)

# Specify how we expect cmake to behave
#cmake_policy(VERSION 3.9)

# ============= Options ==========

# These can be run by calling make test or running the test build step
set(BUILD_TESTING ON CACHE BOOL "Build tests")
# Note Static linking may still have depedencies to external dynamic libs
set(BUILD_SHARED ON CACHE BOOL "Build shared/dynamic runtime libraries (dll)")
# Note Static linking may still have depedencies to external dynamic libs
set(BUILD_STATIC ON CACHE BOOL "Build Static libraries")
# Enable code profiling
set(CODE_PROF OFF CACHE BOOL "Code Profiling")
# Whether the sample code which shows how to use this library should be built
set(BUILD_CQP_EXAMPLES ON CACHE BOOL "Build example code")
# Should we produce package installer(s) for the Toolkit
set(BUILD_INSTALLER ON CACHE BOOL "Build Installer")
# default to a local "test" install
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE STRING "Root of install")
set(DEB_ENABLED ON CACHE BOOL "Build debian/ubuntu package")
# Make extra CMake modules available for use, such as funcitons and find_package(x)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

SET(USE_CLANG OFF CACHE BOOL "Use the CLang compiler")
SET(USE_LLVM_LIBCXX OFF CACHE BOOL "Use the LLVMs libc++ library (requires USE_CLANG)")

if(USE_CLANG)
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    SET(CMAKE_AR      "/usr/bin/llvm-ar")
    SET(CMAKE_LINKER  "/usr/bin/llvm-ld")
    SET(CMAKE_NM      "/usr/bin/llvm-nm")
    SET(CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
    SET(CMAKE_RANLIB  "/usr/bin/llvm-ranlib")
endif()

# This is a super build which pulls together the dependencies for the main project and ensures that they
# are built in the correct order.
# The main project is added at the bottom of this file.
project(CQP NONE)

#HACK:
# I can't get this to work at the package level so it goes here.
SET(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}/src/Algorithms;${CMAKE_BINARY_DIR}/src/CQPToolkit;${CMAKE_BINARY_DIR}/src/QKDInterfaces;${CMAKE_BINARY_DIR}/src/KeyManagement;${CMAKE_BINARY_DIR}/src/Networking;${CMAKE_BINARY_DIR}/src/CQPUI;${CMAKE_BINARY_DIR}/src/IDQDevices")

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

SET(CPACK_PACKAGE_NAME "CQPToolkit")
SET_COMPONENT_NAME("Base" CQP_INSTALL_COMPONENT)

# download dependencies
if(MSVC)
    find_program(NUGET nuget)
    if(NOT NUGET)
        message(STATUS "Downloading Nuget package manager")
        DOWNLOAD("https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${CMAKE_BINARY_DIR}/nuget.exe" "")
        find_program(NUGET nuget HINTS ${CMAKE_BINARY_DIR})
    endif(NOT NUGET)

    if(NUGET)
        message(STATUS "Downloading dependencies from Nuget")
        execute_process(COMMAND ${NUGET} restore "${CMAKE_SOURCE_DIR}" -NonInteractive -PackagesDirectory "${CMAKE_BINARY_DIR}/Packages")

		list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules/Windows")
		include(NugetPackages)
    endif()

    # add qt to the search PATH
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.9.1/msvc2017_64/lib/cmake/Qt5")
endif(MSVC)

if(BUILD_SHARED)
    SET(CQPToolkit_LIB CQPToolkit_Shared CACHE STRING "Name of default toolkit to link to.")

elseif(BUILD_STATIC)
    SET(CQPToolkit_LIB CQPToolkit_Static CACHE STRING "Name of default toolkit to link to.")

endif(BUILD_SHARED)

SET(BUILD_VERSION_MAJOR "0")
SET(BUILD_VERSION_MINOR "4")

find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "libCurl : ${CURL_VERSION_STRING}")

    # Tell our code we have Curl
    add_definitions(-DCURL_FOUND=1)
    add_library(libCurl SHARED IMPORTED)
    set_target_properties(libCurl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIRS}
        IMPORTED_LOCATION ${CURL_LIBRARIES}
    )
else()
    message(ERROR "libCurl not found")
endif()

find_package(CryptoPP)
if(CRYPTOPP_FOUND)
	message(STATUS "Crypto++ : ${CrytpoPP_VERSION}")

        # Tell our code we have Cryptopp
        add_definitions(-DCRYPTOPP_FOUND=1)
else()
	message(WARNING "Crypto++ not found, some projects will be disabled")
endif()

find_package(Subversion "1.8")
if(Subversion_FOUND)
	message(STATUS "Subversion : ${Subversion_VERSION_SVN}")
else()
	message(WARNING "Subversion missing, needed for version tags on release builds.")
endif()

find_package(Git QUIET)
if(Git_FOUND)
	message(STATUS "Git : ${GIT_VERSION_STRING}")
else()
	message(WARNING "Git missing, needed for version tags on release builds.")
endif()

find_package(OpenCL QUIET)
if(OpenCL_FOUND)
        message(STATUS "OpenCL : ${OpenCL_VERSION}${OpenCL_VERSION_STRING}")
        # Tell our code we have OpenCL
        add_definitions(-DOPENCL_FOUND=1)
else()
        message(WARNING "OpenCL missing, some projects will be disabled")
endif()

find_package(Protobuf 3.6.1 QUIET)

pkg_check_modules(SQLITE3 QUIET sqlite3 IMPORTED_TARGET)
if(TARGET PkgConfig::SQLITE3)
    message(STATUS "SQLite3: ${SQLITE3_VERSION}")
    add_definitions(-DSQLITE3_FOUND=1)
endif()

if(${CMAKE_VERSION} VERSION_LESS 3.6)
  if(PROTOBUF_FOUND)
    add_library(protobuf::libprotobuf UNKNOWN IMPORTED)
    set_target_properties(protobuf::libprotobuf PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${PROTOBUF_INCLUDE_DIR}
        IMPORTED_LOCATION ${PROTOBUF_LIBRARY}
    )
  endif()
endif()

if(TARGET protobuf::libprotobuf)
    message(STATUS "libprotobuf: ${Protobuf_VERSION}")
else(TARGET protobuf::libprotobuf)
    message(WARNING "libprotobuf missing.")
endif(TARGET protobuf::libprotobuf)

find_package(GRPC 1.13.1 QUIET)
if(TARGET gRPC::grpc)
    message(STATUS "gRPC++: ${GRPC_VERSION}")

    find_package(OpenSSL)
else(TARGET gRPC::grpc)
    message(WARNING "gRPC++ missing.")
endif(TARGET gRPC::grpc)

find_package(Avahi QUIET)
if(TARGET PkgConfig::avahi-client)
    message("Avahi: ${avahi-client_VERSION}")

    add_definitions(-DAVAHI_FOUND=1)
else()
    message(WARNING "Avahi missing")
endif()

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets QUIET)

if(BUILD_TESTING)
        find_package(GTest QUIET)
        find_package(GMock QUIET)

        if(GTEST_FOUND AND GMOCK_FOUND)
            message(STATUS "GTest : ${GTEST_VERSION}")
            message(STATUS "GMock : ${GMOCK_VERSION}")

        else(GTEST_FOUND AND GMOCK_FOUND)
            message(STATUS "Google Test was not found, it will be built from source")
            # Download the libusb release
            # A special cmake project is used (BuildibUsb.cmake) to find the right library for the platform and install it in a usful place
            ExternalProject_add(GTest_External
                # SVN used so that git does not need to be installed
                # TODO: switch to release tag when its updated.
                SVN_REPOSITORY "https://github.com/google/googletest.git/tags/release-1.8.1"
                    UPDATE_COMMAND ""
                DOWNLOAD_NO_PROGRESS 1
                # Pass arguments to the build process to disable/enable modules
                CMAKE_ARGS
                    -DCMAKE_BUILD_TYPE=Release
                    -Dgtest_force_shared_crt=ON
                    -DBUILD_SHARED_LIBS=OFF
                    -Dgtest_disable_pthreads=${GTEST_DISABLE_PTHREADS}
                    # Install the files in the same location as everything else so that it runs with dlls etc.
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
                    # We dont want to hear about warnings in other peoples builds
                    -Wno-dev
                    ${ExtraGTestArgs}
            )
            # Setup variables as though we'd found gtest

            set(GTEST_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include" CACHE STRING "" FORCE)
            set(GMOCK_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include" CACHE STRING "" FORCE)

            set(GMOCK_LIBRARIES "${CMAKE_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "" FORCE)
            SET(GTEST_LIBRARIES "${CMAKE_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "" FORCE)
            SET(GTEST_MAIN_LIBRARIES "${CMAKE_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE STRING "" FORCE)
            SET(GTEST_FOUND TRUE)
            SET(GMOCK_FOUND TRUE)

            file(MAKE_DIRECTORY "${GMOCK_INCLUDE_DIRS}")
            file(MAKE_DIRECTORY "${GTEST_INCLUDE_DIRS}")
            add_library(gmock STATIC IMPORTED) # or STATIC instead of SHARED
            set_target_properties(gmock PROPERTIES
              IMPORTED_LOCATION "${GMOCK_LIBRARIES}"
              INTERFACE_INCLUDE_DIRECTORIES "${GMOCK_INCLUDE_DIRS}"
            )

            add_library(gtest STATIC IMPORTED) # or STATIC instead of SHARED
            set_target_properties(gtest PROPERTIES
              IMPORTED_LOCATION "${GTEST_LIBRARIES}"
              INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIRS}"
            )

            add_library(gtest_main STATIC IMPORTED) # or STATIC instead of SHARED
            set_target_properties(gtest_main PROPERTIES
              IMPORTED_LOCATION "${GTEST_MAIN_LIBRARIES}"
              INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIRS}"
            )

        endif(GTEST_FOUND AND GMOCK_FOUND)
	
endif(BUILD_TESTING)

find_package(Doxygen)
if(Doxygen_FOUND)
	message(STATUS "Doxygen : ${DOXYGEN_VERSION}")
else()
	message(WARNING "Doxygen missing, Documentation will not be generated")
endif()

find_package(AStyle)
if(ASTYLE_FOUND)
	message(STATUS "AStyle : ${ASTYLE_VERSION}")
else()
	message(WARNING "AStyle missing, prety-print build step disabled")
endif()

if(NOT BUILD_VERSION_PATCH)
    if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        message("Getting build id from git")
        EXEC_PROGRAM("${GIT_EXECUTABLE}" ARGS "-C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD" OUTPUT_VARIABLE GIT_VERSION)
        SET(BUILD_VERSION_PATCH ${GIT_VERSION})

    elseif(Subversion_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.svn")
        message("Getting build id from subversion")
        Subversion_WC_INFO("${CMAKE_HOME_DIRECTORY}" BUILD)
        message("Building revision: ${BUILD_WC_REVISION}")
        SET(BUILD_VERSION_PATCH "${BUILD_WC_REVISION}")

    else()
        message(STATUS "Getting build id from date")
        SET(NEED_FLAG)
        IF(WIN32 AND NOT CYGWIN)
            EXEC_PROGRAM("cmd.exe" ARGS "/C date /T" OUTPUT_VARIABLE CURRENT_DATE)
            STRING(REPLACE "/" ";" CURRENT_DATE ${CURRENT_DATE})
            LIST(REVERSE CURRENT_DATE)
            STRING(CONCAT CURRENT_DATE ${CURRENT_DATE})
        else(WIN32 AND NOT CYGWIN)
            EXEC_PROGRAM("date" ARGS "+%Y%m%d" OUTPUT_VARIABLE CURRENT_DATE)
        ENDIF(WIN32 AND NOT CYGWIN)

        SET(BUILD_VERSION_PATCH "${CURRENT_DATE}")
    endif()
endif(NOT BUILD_VERSION_PATCH)
message("Build time: ${BUILD_VERSION_PATCH}")

SET(BUILD_VERSION "${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}.${BUILD_VERSION_PATCH}")


# configure a version header file with the build version
configure_file (
  "${CMAKE_SOURCE_DIR}/src/Version.cpp.in"
  "${CMAKE_BINARY_DIR}/src/Version.cpp"
  )


# See if lib usb is installed on the system
find_package(LibUsb QUIET)
if(TARGET PkgConfig::libUSB_1)
        message(STATUS "LibUSB-1 : ${libUSB_1_VERSION}")

else(TARGET PkgConfig::libUSB_1)
    message(WARNING "Libusb was not found, projects will be disabled. see README.md")
endif(TARGET PkgConfig::libUSB_1)

find_package(QREncode QUIET)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/idq4p/CMakeLists.txt")

    if(NOT TARGET PkgConfig::MsgPack)
        message("Building MsgPack locally")
        # use a local copy of msgpack
        set(MSGPACK_VERSION "1.1.0" CACHE STRING "The version of MsgPack to use")

        if(MSGPACK_VERSION)
            include(ExternalProject)
            ExternalProject_Add(
                ExMsgPack
                URL "https://github.com/msgpack/msgpack-c/releases/download/cpp-${MSGPACK_VERSION}/msgpack-${MSGPACK_VERSION}.tar.gz"
                URL_HASH MD5=ac41a64d6415fd184215825048bc4523
                UPDATE_COMMAND ""
                CONFIGURE_COMMAND ./configure
                BUILD_COMMAND make
                BUILD_IN_SOURCE true
                INSTALL_COMMAND ""
            )
            # Get the location of the build
            ExternalProject_Get_Property(ExMsgPack SOURCE_DIR)
            # create a folder to prevent error messages on first build
            make_directory("${SOURCE_DIR}/include")
            # create a target for projects to depend on
            add_library(PkgConfig::MsgPack STATIC IMPORTED)
            set_target_properties(PkgConfig::MsgPack PROPERTIES
                IMPORTED_LOCATION "${SOURCE_DIR}/src/.libs/libmsgpack.a"
                INTERFACE_INCLUDE_DIRECTORIES "${SOURCE_DIR}/include")
            add_dependencies(PkgConfig::MsgPack ExMsgPack)
        else()
            # for install system packages
            find_package(MsgPack)
        endif()
    endif()

    if(NOT TARGET PkgConfig::ZeroMQ)
        find_package(ZeroMQ)
    endif()

    if(NOT TARGET Boost)
        find_package(Boost COMPONENTS system)
    endif()

    if(TARGET PkgConfig::ZeroMQ AND TARGET Boost::system)
        message("Adding idq4p to build")
        SET_COMPONENT_NAME("Drivers" CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)
        add_subdirectory(external/idq4p)
        SET_COMPONENT_NAME("Base" CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)
    else()
        message(WARNING "Not building idq4p due to missing dependencies")
    endif()
endif()

if(BUILD_TESTING)
	# This turns on some testing features of CMake, it doesn't magically 'do testing' :(
	enable_testing()
	include (CTest)	

	if(CYGWIN)
	    # Remove the warning about cmake versions
		list(APPEND ExtraGTestArgs "-DCMAKE_LEGACY_CYGWIN_WIN32=1" "gtest_disable_pthreads=OFF")
	elseif(MSVC)
		set(GTEST_MSVC_SEARCH "MT")
	endif(CYGWIN)

    if(MSVC)
        set(GTEST_DISABLE_PTHREADS ON)
    else()
        set(GTEST_DISABLE_PTHREADS OFF)
    endif()
endif(BUILD_TESTING)

if(Doxygen_FOUND)
    # The Doxygen.in file has a placeholder for this variable which will be substituted at build time.
    set(HAVE_DOT "NO") # unless we find it later
    set(HAVE_PLANTUML "NO") # unless we find it later

    # TODO: Fix for windows
    if(NOT WIN32 AND DOXYGEN_DOT_FOUND)
        set(HAVE_DOT "YES")
    endif(NOT WIN32 AND DOXYGEN_DOT_FOUND)

    # add a target to generate API documentation with Doxygen
    # to get this to work with visual studio see: http://www.mantidproject.org/Visual_Studio_Doxygen_Setup

    find_file(PLANTUML_JARFILE
         NAMES plantuml.jar
         HINTS "/usr/share/java/plantuml" "/opt/plantuml" "/usr/share/plantuml" ENV PLANTUML_DIR
       )
   include(FindPackageHandleStandardArgs)
   find_package_handle_standard_args(
     PLANTUML DEFAULT_MSG PLANTUML_JARFILE)
	if(NOT PLANTUML_JARFILE)
    	# stop doxygen complaining about missing plantuml
    	set(PLANTUML_JARFILE "")
    endif()

    get_filename_component(DOT_PATH ${DOXYGEN_DOT_EXECUTABLE} DIRECTORY CACHE)
    configure_file(${PROJECT_SOURCE_DIR}/proto2cpp.py ${CMAKE_BINARY_DIR}/proto2cpp.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
    file(GLOB DocFiles LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.md" "*.rtf" "*.txt" "LICENSE")
    add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        SOURCES ${PROJECT_SOURCE_DIR}/Doxyfile.in ${DocFiles}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
        BYPRODUCTS "${CMAKE_BINARY_DIR}/doc/html/"
    )
    
    # Add this library to the list of things to install
    # The trailing slash is inportant on the DIRECTORY parameter
    # without it the destination will be /doc/doc/
    # This will create an option in the installer
#BUG: This causes the package to be invalid
    ##INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/doc/html/" DESTINATION doc
    ##COMPONENT ${PROJECT_NAME}_Docs)

    if(MSVC)
        set_property(TARGET doc PROPERTY FOLDER "Documentation")
        set_property(TARGET doc PROPERTY EXCLUDE_FROM_DEFAULT_BUILD 1)
    endif(MSVC)

    if(DOXYGEN_DOT_FOUND)

        # Build a graph of the build dependencies - usful for debugging build issues
        add_custom_target(depgraph COMMAND
            ${CMAKE_COMMAND} ${PROJECT_SOURCE_DIR} --graphviz=${CMAKE_BINARY_DIR}/doc/dependencies.dot 
        )
        # make the doc target also create the dep graph
        add_dependencies(doc depgraph)
        if(MSVC)
            set_property(TARGET depgraph PROPERTY FOLDER "Documentation")
        set_property(TARGET depgraph PROPERTY EXCLUDE_FROM_DEFAULT_BUILD 1)
        endif(MSVC)

    endif(DOXYGEN_DOT_FOUND)

endif(Doxygen_FOUND)

find_package(AStyle QUIET)
if(ASTYLE_FOUND)
    # Add a custom target
    add_custom_target("style" COMMAND
      "${ASTYLE_EXECUTABLE}"
      --options="${CMAKE_CURRENT_SOURCE_DIR}/general.astylerc" -r \"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp\" -r \"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp\" -r \"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h\"
    )
    if(MSVC)
        set_property(TARGET style PROPERTY FOLDER "Preprocessing")
    endif(MSVC)
endif(ASTYLE_FOUND)

# add some files used in the build which aren't "built"
file(GLOB ExtraFiles LIST_DIRECTORIES false RELATIVE "${PROJECT_SOURCE_DIR}" "*.xslt" "*.xml" "*.yml" "*.astylerc" "Dockerfile")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/extra)
foreach(_xFile ${ExtraFiles})
    configure_file(${_xFile} extra/ COPYONLY)
endforeach()

if(EXISTS ${CMAKE_SOURCE_DIR}/external/nodeeditor/CMakeLists.txt)

    SET_COMPONENT_NAME("exclude" CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)
    SET(BUILD_SHARED_LIBS OFF)
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/nodeeditor)
    SET_COMPONENT_NAME("" CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)

endif()
# Setup is done, now include the source code to build.
add_subdirectory(src)


if(BUILD_INSTALLER)

    if( MINGW )
        message( STATUS "Installing system-libraries: MinGW DLLs." )
        get_filename_component( Mingw_Path ${CMAKE_CXX_COMPILER} PATH )
        LIST(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS 
            ${Mingw_Path}/libgcc_s_seh-1.dll 
            ${Mingw_Path}/libstdc++-6.dll
            ${Mingw_Path}/libwinpthread-1.dll
            ${Mingw_Path}/libpcre-1.dll
            ${Mingw_Path}/zlib1.dll
            ${Mingw_Path}/libexpat-1.dll )
    endif(MINGW)

    message(STATUS "The following runtime dlls will be included:")
    foreach(DLL ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        message(STATUS "   ${DLL}")
    endforeach(DLL)

    if(MINGW OR MSVC)
        # This prevents the InstallRequiredSystemLibraries macro from doing the install so we can specify other parameters
        # In this case we're using it to populate the variable with all the dependencies
        SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        # This only has an effect on MSVC compilers
        include( InstallRequiredSystemLibraries )
        install( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin COMPONENT RunTimeDeps )
    endif( MINGW OR MSVC )

    include(Packaging)
endif(BUILD_INSTALLER)
