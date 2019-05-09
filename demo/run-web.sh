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
docker chromium-qkd 2>&1 2>/dev/null

rm site-a.db* site-b.db*

# Top left
byobu new-session -d -s "${SESSION}" -c . "${SITEAGENT} -c site-a.json"
byobu rename-window Sites
# bottom left
byobu split-window -v "${SITEAGENT} -c site-b.json"
byobu select-pane -t 0
# top right
byobu split-window -h "${DRIVER}" -q -c qkd-alice.json
byobu select-pane -D -t 0
# bottom right
byobu split-window -h "${DRIVER}" -q -c qkd-bob.json
sleep 1
byobu new-window -n WebServer "docker run -it --rm -v `pwd`/nginx-conf:/etc/nginx:ro --net=host --name nginx-qkd \
    --entrypoint=/usr/sbin/nginx \
    registry.gitlab.com/qcomms/cqptoolkit/nginx-qkd"
byobu split-window -h ../external/chromium/run-chromium.sh --pskhsm=https://localhost:8001 http://localhost:8080 https://localhost:8433

byobu attach
