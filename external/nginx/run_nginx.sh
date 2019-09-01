#!/bin/bash
SUDO=sudo
if [ "$UID" == "0" ]; then
	# sudo might not exist, we dont need it anyway
	SUDO=""
fi

if [ -f /.dockerenv ]; then
    if [ "$IFACE" != "" ]; then
	    # inside docker
	    echo -n "Waiting for interface $IFACE..."
	    /pipework/pipework --wait -i "$IFACE" && \
	    /usr/sbin/dnsmasq && \
    	nginx
    else
	    service php7.2-fpm start
	    nginx
	fi
else 
	if [ "$1" == "" ]; then
		echo "Please specify host interface to link to the container"
		exit 1
	fi
	if [ ! -d pipework ]; then
		git clone https://github.com/jpetazzo/pipework.git
	fi
	# the -i keeps the interface name the same inside and out of the container
	$SUDO ./pipework/pipework --direct-phys "$1" -i "$1" $(docker run -td -v /var/lib/softhsm:/var/lib/softhsm --rm --name nginx --env IFACE="$1" qkdsecure) 192.168.201.6/24
fi
	
