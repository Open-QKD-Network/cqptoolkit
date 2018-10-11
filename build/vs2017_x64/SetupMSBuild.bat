:: Copyright (C) University of Bristol 2017
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
set local
set PATH=%PATH%;C:\Program Files (x86)\CMake\bin
cmake -G "Visual Studio 15 2017 Win64" --build . ..\.. %*
pause