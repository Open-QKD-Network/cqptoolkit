#!/bin/bash
# Copyright (C) University of Bristol 2016
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Date 04 April 2016
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
USBDEV=`lsusb -d 1ddc: | sed s/:// | awk '{ print ("/dev/bus/usb/" $2 "/" $4) }'`
XAUTHORITY=$HOME/.Xauthority 
DEBUG=0
BUILD=0
OPTIONS="--rm --name CQPToolkit-$PPID --network=host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v $HOME/.Xauthority:/root/.Xauthority"

if [ ! -z "$USBDEV" ]; then
	echo "Usb device: \"$USBDEV\""
	OPTIONS="${OPTIONS} --device $USBDEV"
else 
	echo "WARNING: no valid usb device found!"
fi

while [[ $# -gt 0 ]]; do
key="$1"

case $key in
    -h|--help)
		echo "-h Help"
		echo "-d Debug"
		echo "-b Build image first"
		exit 0
    	;;
	-d)
		DEBUG=1
    	;;
    -b)
		BUILD=1
		;;
	*)
		echo "Unknown option"
		exit 1
    	;;
esac
shift # past argument or value
done

if [ ${DEBUG} -eq 1 ]; then
	OPTIONS="${OPTIONS} -it --entrypoint=/bin/bash"
fi

if [ ${BUILD} -eq 1 ]; then
	sudo docker build -t cqp-dev.phy.bris.ac.uk:5000/cqp/toolkit .
	if [ $? -gt 0 ]; then
		exit $?
	fi
fi
sudo docker run ${OPTIONS} cqp-dev.phy.bris.ac.uk:5000/cqp/toolkit

