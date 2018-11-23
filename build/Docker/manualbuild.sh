#!/bin/bash
# Copyright (C) University of Bristol 2017
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
if [ -d "$1" ]; then
	DIR="$1"
else
	DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"/../../
fi

if [ -f "${DIR}/CMakeLists.txt" ]; then
        sudo docker run -it --rm -v ${DIR}:/builds/CQP/CQPToolkit registry.gitlab.com/qcomms/cqptoolkit/buildenv:latest /bin/bash -c 'cd /builds/CQP/CQPToolkit/build/gcc && cmake ../.. && make -j6 -s package'
else 
	echo "Cannot find source dir, pass it on commandline"
fi
