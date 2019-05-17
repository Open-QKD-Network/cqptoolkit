#!/bin/bash
if [ "$1" == "" ]; then
	BUILDDIR=../../build-cqptoolkit-Desktop-Default/src

	SITEAGENT=`which SiteAgentRunner 2>/dev/null`
	DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer

	if [ "$SITEAGENT" == "" ]; then
		SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
	fi
	if [ "$DRIVER" == "" ]; then
		DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	fi
	if [ "QTUN" == "" ]; then
		QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
	fi
else
	BUILDDIR="$1"

	SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
	DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
fi

SESSION=webdemo

# Cleanup
byobu kill-session -t ${SESSION}
docker stop nginx-qkd 2>&1 2>/dev/null
docker stop stunnel-qkd 2>&1 2>/dev/null

rm site-a.db* site-b.db*

# Top left
byobu new-session -d -s "${SESSION}" -c . "${SITEAGENT} -c site-a.json -v || /bin/bash"
byobu rename-window Sites

# bottom left
byobu split-window -v "${SITEAGENT} -c site-b.json -v || /bin/bash"
byobu select-pane -t 0
# top right
byobu split-window -h "${DRIVER} -q -c qkd-alice.json || /bin/bash"
byobu select-pane -D -t 0
# bottom right
byobu split-window -h "${DRIVER} -q -c qkd-bob.json || /bin/bash"
sleep 2
byobu new-window -n WebServer "docker run -it --rm -v `pwd`/nginx-conf:/etc/nginx:ro --net=host --name nginx-qkd \
    --entrypoint=/usr/sbin/nginx \
    registry.gitlab.com/qcomms/cqptoolkit/nginx-qkd || /bin/bash"
sleep 1
byobu split-window -h "docker run -it --rm --net=host --name stunnel-qkd registry.gitlab.com/qcomms/cqptoolkit/stunnel-qkd || /bin/bash"

# launch browser
xdg-open http://localhost:80
# this will only work with a compatible browser
#xdg-open https://localhost:8443

byobu attach -t "${SESSION}"

docker stop nginx-qkd 2>&1 2>/dev/null
docker stop stunnel-qkd 2>&1 2>/dev/null
