:: Copyright (C) University of Bristol 2017
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
:: TODO: Doesn't build because the developer tools are not in the path
if exist "%VS140COMNTOOLS%\VsDevCmd.bat" (
    call "%VS140COMNTOOLS%\VsDevCmd.bat"
) else (
    if exist "%VS140COMNTOOLS%\VsMSBuildCmd.bat" (
        call "%VS140COMNTOOLS%\VsMSBuildCmd.bat"
    )
)

cmake ..\.. -G "Sublime Text 2 - NMake Makefiles"
pause