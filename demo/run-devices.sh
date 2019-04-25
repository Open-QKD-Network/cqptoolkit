#!/bin/bash
BUILDDIR=../build-cqptoolkit-Desktop-Default/src
SITEAGENT=${BUILDDIR}/Tools/SiteAgentRunner/SiteAgentRunner
DRIVER=${BUILDDIR}/Drivers/DummyQKDDriver/DummyQKDDriver
QTUN=${BUILDDIR}/Tools/QTunnelServer/QTunnelServer
SESSION=driverdemo

byobu kill-session -t ${SESSION}
# left
byobu new-session -d -s "${SESSION}" -c . "${DRIVER} -b -k localhost:7000"
byobu rename-window Drivers
# right
byobu split-window -h "${DRIVER} -a -m localhost:7000" 
byobu select-pane -t 0 

byobu attach
