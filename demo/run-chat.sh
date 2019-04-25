#!/bin/bash
BUILDDIR=../build-cqptoolkit-Desktop-Default/src
SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
SESSION=demo3

byobu kill-session -t ${SESSION}
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
byobu new-window -n VPN "${QTUN}" -c VPN-b.json
byobu split-window -v "${QTUN}" -c VPN-a.json -a
byobu select-pane -t 0
byobu split-window -h "nc -t localhost 9000"
byobu select-pane -D -t 0
byobu split-window -h "nc -t localhost 9001"

byobu attach
