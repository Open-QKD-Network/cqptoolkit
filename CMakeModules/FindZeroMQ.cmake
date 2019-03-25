### @file
### @brief CQP Toolkit
###
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 24 Feb 2019
### @author Richard Collins <richard.collins@bristol.ac.uk>
###
# Use the package PkgConfig to detect headers/library files
find_package(PkgConfig QUIET)
if(${PKG_CONFIG_FOUND})
        pkg_check_modules(ZeroMQ QUIET libzmq IMPORTED_TARGET)
        if(TARGET PkgConfig::ZeroMQ)
            message("ZeroMQ Found: ${ZeroMQ_VERSION}")
        endif()
endif(${PKG_CONFIG_FOUND})
