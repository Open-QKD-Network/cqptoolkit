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
# Use the package PkgConfig to detect headers/library files

if(NOT CRYPTOPP_FOUND)
    find_path(ARGGG
        NAMES cryptopp/cryptlib.h
        PATH_SUFFIXES include
    )
    set(CRYPTOPP_INCLUDEDIR ${ARGGG})

    find_library(CRYPTOPP_LIBRARIES NAMES cryptopp cryptopp.dll
        PATH_SUFFIXES "lib/native/v140/windesktop/msvcstl/${MSVC_C_ARCHITECTURE_ID}/${CMAKE_BUILD_TYPE}/md")

    if(CRYPTOPP_INCLUDEDIR AND CRYPTOPP_LIBRARIES)
        if(WIN32)
            find_file( CRYPTOPP_DLLS
                NAMES libcryptopp.dll
                PATHS ${CMAKE_LIBRARY_PATH} ${CMAKE_PROGRAM_PATH}
                PATH_SUFFIXES
                "lib/native/${MSVC_C_ARCHITECTURE_ID}"
                "bin"
            )
            if(CRYPTOPP_DLLS)
                file(COPY ${CRYPTOPP_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
                LIST(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${CRYPTOPP_DLLS}")
            endif()
        endif(WIN32)
        set(CRYPTOPP_FOUND TRUE)

        add_library(cryptopp SHARED IMPORTED)
        set_target_properties(cryptopp PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${CRYPTOPP_INCLUDEDIR}
            IMPORTED_LOCATION ${CRYPTOPP_LIBRARIES}
        )
    elseif(CRYPTOPP_INCLUDEDIR AND NOT CRYPTOPP_LIBRARIES)

        find_library(CRYPTOPP_LIBRARIES NAMES cryptopp-static
            PATH_SUFFIXES "lib/native/v140/windesktop/msvcstl/${MSVC_C_ARCHITECTURE_ID}/${CMAKE_BUILD_TYPE}/md")

        if(CRYPTOPP_LIBRARIES)
            set(CRYPTOPP_FOUND TRUE)

            add_library(cryptopp STATIC IMPORTED)
            set_target_properties(cryptopp PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES ${CRYPTOPP_INCLUDEDIR}
                IMPORTED_LOCATION ${CRYPTOPP_LIBRARIES}
            )
        endif()
    endif()
endif(NOT CRYPTOPP_FOUND)

if(CRYPTOPP_FOUND)
    message("Looking for RDRand in ${CRYPTOPP_INCLUDEDIR}...")
    if(EXISTS "${CRYPTOPP_INCLUDEDIR}/cryptopp/rdrand.h")
        message("found.")
        SET(CRYPTOPP_HASRDRAND TRUE)
        add_definitions(-DCRYPTOPP_HASRDRAND=1)
    endif()
endif(CRYPTOPP_FOUND)
