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

echo Enabling source repositories...
$SUDO sed -i '/^#\sdeb-src /s/^#//' "/etc/apt/sources.list"
$SUDO apt update -q && $SUDO apt install -qy dpkg-dev
echo Getting nginx source...

apt source nginx && \
$SUDO apt build-dep -qy nginx && \
pushd `find -maxdepth 1 -type d -name 'nginx-*'`
quilt import -P cqptoolkit.patch ../cqptoolkit.patch && \
sed -i -e 's/--with-threads$/--with-threads \\\n\t\t\t--with-http_ssl_psk/' debian/rules ||true && \
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage --no-sign
popd

