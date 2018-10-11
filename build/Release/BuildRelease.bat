@echo off
::Copyright (C) University of Bristol 2017
::   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::   If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::   See LICENSE file for details.
PATH=C:\MSYS64\mingw64\bin;%PATH%
"c:\Program Files (x86)\CMake\bin\cmake.exe" -G "MinGW Makefiles" --build . ..\.. -DCMAKE_BUILD_TYPE=Release
mingw32-make -j all doc package bundle
"c:\Program Files\7-Zip\7z.exe" a -bd -mmt doc.7z doc/
svn export ../.. sourcecode
"c:\Program Files\7-Zip\7z.exe" a -bd -mmt sourcecode.7z sourcecode
