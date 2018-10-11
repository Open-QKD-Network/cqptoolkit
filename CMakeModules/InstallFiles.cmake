### @file
### @brief CQP Toolkit - Put files in a standard path
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

# Configures post build events to install files into the build tree
function(InstallFiles srcDir destDir)
    # find files of interest
    file(GLOB_RECURSE includeFiles ${srcDir}/*.h)
    file(GLOB_RECURSE libFiles ${srcDir}/*${CMAKE_IMPORT_LIBRARY_SUFFIX})
    file(GLOB_RECURSE binFiles ${srcDir}/*${CMAKE_SHARED_LIBRARY_SUFFIX})
    file(GLOB_RECURSE libFiles ${srcDir}/*${CMAKE_STATIC_LIBRARY_SUFFIX})

    foreach(folder "${destDir}/include" "${destDir}/bin" "${destDir}/lib")
        if(NOT EXISTS "${folder}")
            # ensure destination folders exist
            file(MAKE_DIRECTORY "${folder}")
        endif(NOT EXISTS "${folder}")
    endforeach(folder)
    
    # copy the files
    InstallFiles_From("${includeFiles}" "${destDir}/include")
    InstallFiles_From("${binFiles}" "${destDir}/bin")
    InstallFiles_From("${libFiles}" "${destDir}/lib")
endfunction(InstallFiles)

function(InstallFiles_From files destDir)
    foreach(templateFile ${files})
        message(STATUS "Installing ${templateFile} -> ${destDir}")
        file(INSTALL ${templateFile} DESTINATION ${destDir})
    endforeach(templateFile)
endfunction(InstallFiles_From)