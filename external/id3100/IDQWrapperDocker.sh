#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
NETNAME=idq-ipc
IMAGE=cqp-dev.phy.bris.ac.uk:5000/cqp/idqcontainer
IDQPRODUCT=1DDC:
exposedPort=7030

SUDO=sudo

if [ "$UID" == "0" ]; then
  SUDO=""
fi

# look for the network interface
sudo docker network inspect -f "{{.Name}}" ${NETNAME} 2>&1 > /dev/null
NETEXISTS=$?

# if the inspect returned 0, the network interface already exists
if [ ${NETEXISTS} -ne 0 ]; then
    # create the interface
    echo "Creating dedicated swarm network..."
    # attachable lets normal containers attach to it
    # internal disconnects it from any exteral interface - broken: prevents publishing ports
    sudo docker network create -d overlay --attachable ${NETNAME} --subnet 192.168.103.0/24
fi

${SUDO} docker pull ${IMAGE}

OPTS=$@

lsusb -d ${IDQPRODUCT} | while read line ; do
    set ${line}
    BUS=$2
    DEV=$4
    PROD=$6
    # take the firt 3 characters from 0 position
    DEV=${DEV:0:3}
    # take 4 characters from pos 5
    PROD=${PROD:5:4}
    if [ "${PROD}" = "0203" ] ; then
        SIDE=Alice
    else
        SIDE=Bob
    fi

    NAME=idqcontainer-${SIDE}-${BUS}-${DEV}
    HOSTN=`hostname -s`-IDQ-${BUS}-${DEV}

    ${SUDO} docker stop ${NAME} 2> /dev/null
    ${SUDO} docker rm ${NAME} 2> /dev/null

    echo "Running ${SIDE} container called ${NAME} with hostname ${HOSTN}. Port 7000 -> ${exposedPort}..."
    # see https://docs.docker.com/engine/reference/run/#runtime-privilege-and-linux-capabilities
    ${SUDO} docker run -t \
        --name ${NAME} \
        --hostname ${HOSTN} \
        --restart unless-stopped \
        -p 0.0.0.0:${exposedPort}:7000 \
        --expose 7000 \
        --network ${NETNAME} \
        --device=/dev/bus/usb/$BUS/$DEV:/dev/bus/usb/$BUS/$DEV \
        ${IMAGE} $OPTS &

    let "exposedPort += 1"
    #echo "Ports published:"
    #${SUDO} docker port ${NAME}
    #--privileged
    #--cidfile="${NAME}.id" \
    #--cap-add SYS_RAWIO \
    #-v /dev/bus/usb:/dev/bus/usb \

done

if [ "$DEV" == "" ]; then
  echo "No clavis devices found."
  exit 1
fi
