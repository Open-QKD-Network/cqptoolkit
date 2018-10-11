#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
IMAGE=yobihsm
YUBIPRODUCT=1050:

OPTS=$@

#SUDO=sudo

if [ "$UID" == "0" ]; then
  SUDO=""
fi


lsusb -d ${YUBIPRODUCT} | while read line ; do
    set ${line}
    BUS=$2
    DEV=$4
    PROD=$6
    # take the firt 3 characters from 0 position
    DEV=${DEV:0:3}
    # take 4 characters from pos 5
    PROD=${PROD:5:4}
    if [ "${PROD}" = "0030" ] ; then
        # HSM2

        NAME=yubihsm2-${BUS}-${DEV}

        ${SUDO} docker stop ${NAME} 2> /dev/null
        ${SUDO} docker rm ${NAME} 2> /dev/null

        echo "Running container called ${NAME}..."
        # see https://docs.docker.com/engine/reference/run/#runtime-privilege-and-linux-capabilities
        echo ${SUDO} "docker run -it --rm \
            --name ${NAME} \
            --network=host \
            --device=/dev/bus/usb/$BUS/$DEV:/dev/bus/usb/$BUS/$DEV \
            ${IMAGE} $OPTS"

    fi

done

