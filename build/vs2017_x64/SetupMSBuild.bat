:: Copyright (C) University of Bristol 2017
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
echo "Ensure vcpkg is setup on your system by running InstallVcPkg.bat"
set local
set PATH=%PATH%;C:\Program Files (x86)\CMake\bin;C:\vcpkg
vcpkg update
:: QT5 doesn't build if the path to vcpkg is too long
vcpkg install --triplet x64-windows grpc cryptopp curl opencl libusb sqlite3 libqrencode
::vcpkg install --triplet x64-windows qt5-base
cmake -G "Visual Studio 15 2017 Win64" --build . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..\.. %*
pause
