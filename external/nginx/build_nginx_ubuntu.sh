#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
NGINX_VERSION=1.14.0

apt source nginx && \
cp cqptoolkit_ubuntu.patch nginx-${NGINX_VERSION}/debian/patches && \
echo "cqptoolkit_ubuntu.patch" >> nginx-${NGINX_VERSION}/debian/patches/series && \
sed -i -e 's/--with-threads$/--with-threads \\\n\t\t\t--with-http_ssl_psk/' nginx-${NGINX_VERSION}/debian/rules && \
cd nginx-${NGINX_VERSION} && \
dpkg-buildpackage --no-sign



