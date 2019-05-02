#!/bin/bash
if [ "$1" == "" ]; then
	BUILDDIR=../../build-cqptoolkit-Desktop-Default/src

	SITEAGENT=`which SiteAgentRunner 2>/dev/null`
	DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
        STATS=${BUILDDIR}/Tools/StatsDump/StatsDump

	if [ "$SITEAGENT" == "" ]; then
		SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
	fi
	if [ "$DRIVER" == "" ]; then
		DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	fi
	if [ "QTUN" == "" ]; then
		QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
	fi
        if [ "STATS" == "" ]; then
                STATS=${BUILDDIR}/Tools/StatsDump/StatsDump
        fi
else 
	BUILDDIR="$1"

	SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
	DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
	QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
        STATS=${BUILDDIR}/Tools/StatsDump/StatsDump
fi

SESSION=driverdemo

byobu kill-session -t ${SESSION}
# left
byobu new-session -d -s "${SESSION}" -c . "${DRIVER} -b -k localhost:7000"
byobu rename-window Drivers

byobu split-window -v "${STATS} -c localhost:7000"

byobu select-pane -t 0
# right
byobu split-window -h "${DRIVER} -a -m localhost:7000" 
byobu select-pane -t 0

byobu attach
