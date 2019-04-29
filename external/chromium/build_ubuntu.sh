#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
#

SUDO=sudo
if [ "$UID" == "0" ]; then
  SUDO=""
fi

echo Enablng source repositories...
$SUDO sed -i '/^#\sdeb-src /s/^#//' "/etc/apt/sources.list"
$SUDO apt update -q && $SUDO apt install -qy dpkg-dev
echo Getting Chromium source...

apt source chromium-browser && \
echo Installing buld dependencies... && \
$SUDO apt build-dep -qy chromium-browser && \
$SUDO apt install -qy qtbase5-dev && \
pushd `find -type d -name "chromium-browser-*"` && \
echo Applying patch && \
quilt import -P cqptoolkit-deb.patch ../cqptoolkit-deb.patch ; \
sed -i -e 's/optimize_webui=false$/optimize_webui=false \\\n\tuse_psk=true/' debian/rules && \
echo Building... && \
dpkg-buildpackage --no-sign -nc
popd
