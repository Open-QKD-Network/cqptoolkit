@echo off
:: Copyright (C) University of Bristol 2018
::    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
::    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
::    See LICENSE file for details.
:: Author Richard Collins <richard.collins@bristol.ac.uk>
:: 
call git clone https://github.com/plasticbox/grpc-windows.git
cd grpc-windows
grpc_clone.bat
grpc_build.bat

pause
