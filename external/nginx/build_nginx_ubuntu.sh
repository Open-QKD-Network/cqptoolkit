#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 

apt source nginx && \
apt build-dep nginx && \
cd `find -maxdepth 1 -type d -name 'nginx-*'` && \
quilt import -P cqptoolkit_ubuntu.patch ../cqptoolkit_ubuntu.patch && \
sed -i -e 's/--with-threads$/--with-threads \\\n\t\t\t--with-http_ssl_psk/' debian/rules ||true && \
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage --no-sign -j4


#cp cqptoolkit_ubuntu.patch nginx-${NGINX_VERSION}/debian/patches && \
#echo "cqptoolkit_ubuntu.patch" >> nginx-${NGINX_VERSION}/debian/patches/series && \

