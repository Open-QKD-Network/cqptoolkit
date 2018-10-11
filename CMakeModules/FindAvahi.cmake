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

find_package(PkgConfig)
if(PkgConfig_FOUND)
    #pkg_check_modules(avahi-core avahi-core QUIET IMPORTED_TARGET)
    pkg_check_modules(avahi-client QUIET IMPORTED_TARGET avahi-client)
    #add_dependencies(PkgConfig::avahi-client PkgConfig::avahi-core)

    if(${CMAKE_VERSION} VERSION_LESS 3.6)
    	find_library(AVAHI_LIBRARY-COMMON NAMES avahi-common)
		find_library(AVAHI_LIBRARY-CLIENT NAMES avahi-client)
		find_path(AVAHI_INCLUDE_DIR avahi-client/publish.h)
		include(FindPackageHandleStandardArgs)
		find_package_handle_standard_args(Avahi DEFAULT_MSG AVAHI_LIBRARY-COMMON AVAHI_LIBRARY-CLIENT AVAHI_INCLUDE_DIR)
		if(AVAHI_FOUND)
		    set(AVAHI_LIBRARIES ${AVAHI_LIBRARY-COMMON} ${AVAHI_LIBRARY-CLIENT})
		    set(AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDE_DIR})

			add_library(PkgConfig::avahi-client UNKNOWN IMPORTED)
			set_target_properties(PkgConfig::avahi-client PROPERTIES
			    INTERFACE_INCLUDE_DIRECTORIES "${AVAHI_INCLUDE_DIRS}"
			    IMPORTED_LOCATION "${AVAHI_LIBRARIES}"
			)
		endif()
    endif()

else()
	message(WARNING "No pkgconfig")
endif(PkgConfig_FOUND)
