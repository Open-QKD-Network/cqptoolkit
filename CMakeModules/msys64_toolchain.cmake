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
# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
set(TOOLCHAIN_PREFIX "x86_64-w64-mingw32")

set(CMAKE_MAKE_PROGRAM "mingw32-make")
# cross compilers to use for C and C++
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER windres)
set(CMAKE_LINKER ld)
set(CMAKE_STRIP strip)
set(CMAKE_AR ar)
set(CMAKE_NM nm)
set(CMAKE_OBJCOPY objcopy)
set(CMAKE_OBJDUMP objdump)


# target environment on the build host system
#   set 1st to dir with the cross compiler's C/C++ headers/libs
set(CMAKE_FIND_ROOT_PATH "c:/msys64/mingw64")

# modify default behavior of FIND_XXX() commands to
# search for headers/libs in the target environment and
# search for programs in the build host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)