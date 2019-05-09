#!/bin/bash
docker run -it --rm --name chromium-qkd --user $(id -u):$(id -g) \
    --privileged \
    --net=host \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $HOME/.Xauthority:/home/.Xauthority:ro \
    --shm-size=2g \
    registry.gitlab.com/qcomms/cqptoolkit/chromium-qkd /usr/bin/chromium-browser $@
