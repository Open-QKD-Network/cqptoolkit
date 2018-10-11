@echo off
:: Copyright (C) University of Bristol 2017
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
PATH=C:\MSYS64\bin;%PATH%
:: Testing is currently not working on MSYS/mingw
cmake -G "MSYS Makefiles" --build . ..\.. %*
pause