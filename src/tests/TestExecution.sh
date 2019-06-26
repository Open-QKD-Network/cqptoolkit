#!/bin/bash
for debFile in CQPToolkit-*Base*.deb CQPToolkit-*Drivers*.deb ; do
    files=`dpkg -c $debFile | grep -e "^-r.xr.xr.x" | awk '{print $6}'`
    if [ "$files" != "" ]; then
        for exeFile in `basename -a $files`; do
            echo "Running $exeFile..."
            $exeFile --version || exit $?
        done
    fi
done
