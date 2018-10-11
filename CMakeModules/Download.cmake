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
# Perform a download step at build time.
SET(DOWNLOAD_URL "" CACHE STRING "file to download")
SET(DOWNLOAD_FILE "" CACHE STRING "destination file name")
SET(DOWNLOAD_MD5 "" CACHE STRING "MD5 of file")

file(DOWNLOAD "${DOWNLOAD_URL}" "${DOWNLOAD_FILE}" SHOW_PROGRESS EXPECTED_MD5 "${DOWNLOAD_MD5}"
    INACTIVITY_TIMEOUT 60
    STATUS _downloadStatus)

LIST(GET _downloadStatus 0 _downloadExitCode)
if(NOT _downloadExitCode EQUAL 0)
    message(FATAL_ERROR "Failed to download ${DOWNLOAD_FILE}, removing stale file")
    file(REMOVE "${DOWNLOAD_FILE}")
endif()
