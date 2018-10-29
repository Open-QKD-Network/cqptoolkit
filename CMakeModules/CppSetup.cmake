### @file
### @brief CQP Toolkit - Standard setup macro for C++
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

enable_language(CXX)

#============ Compiler version chaeck ========
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
    message(FATAL_ERROR "Insufficient gcc version: ${CMAKE_CXX_COMPILER_VERSION}")
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
# The x is added to stop MSVC being interpreted as a variable
# See CMake CMP0054 for details
elseif(MSVC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
    message(FATAL_ERROR "Insufficient msvc version")
  endif()
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID} - the build may fail.")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(USE_LLVM_LIBCXX)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif(USE_LLVM_LIBCXX)
    endif()

    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 -fPIC ")
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_PCAP")
    if(NOT CYGWIN)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread")
    endif(NOT CYGWIN)

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -Wall")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_DEBUG -Wall")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_DEBUG -Wall")

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELWITHDEBINFO} -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall")

    if(CODE_PROF)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg" )
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pg")
    endif(CODE_PROF)

    set(CMAKE_DEBUG_POSTFIX "d")

    get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
    set(LIBSUFFIX "-${CMAKE_SYSTEM_PROCESSOR}")

elseif(MSVC)
    # Set compiler flags and options.
    # Here it is setting the Visual Studio warning level to 4
    # Disable warning 4251 [explanation](http://www.microsoft-questions.com/microsoft/VC-Language/30952961/a-solution-to-warning-c4251--class-needs-to-have-dllinterface.aspx)
    #  Be aware it may hide the cause of linker errors
    # Disable warning 4250 [explanation](https://msdn.microsoft.com/en-us/library/6b3sy7ae.aspx) related to virtual inheritence
    #  This is used for implementation of interfaces and the warning is unnessacary.
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /MDd /wd4251 /wd4250")

    # Set compiler flags and options.
    if(CODE_PROF)
        Message(WARNING "Profiling with msbuild is no currently implemented, please use visual studio.")
    endif(CODE_PROF)
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
ENDIF()

# Tell the compiler to look in the current folder (relative to the source tree) for header files
include_directories (.)
include_directories (${CMAKE_INCLUDE_OUTPUT_DIRECTORY})
