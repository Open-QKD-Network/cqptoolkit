:: Copyright (C) University of Bristol 2017
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
echo Installing vcpkg to user local app data: %LOCALAPPDATA%
set local
pushd c:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
call bootstrap-vcpkg.bat
vcpkg integrate install
popd
pause