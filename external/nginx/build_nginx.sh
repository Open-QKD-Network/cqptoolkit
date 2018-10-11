#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
if [ ! -d "nginx" ]; then
	hg clone http://hg.nginx.org/nginx
fi
pushd nginx && \
hg import --no-commit ../cqptoolkit.patch && \
./auto/configure --with-compat --with-debug --with-file-aio --with-http_addition_module --with-http_auth_request_module \
 --with-http_dav_module --with-http_degradation_module --with-http_flv_module --with-http_geoip_module \
 --with-http_gunzip_module --with-http_gzip_static_module --with-http_mp4_module --with-http_realip_module \
 --with-http_secure_link_module --with-http_slice_module --with-http_ssl_module --with-http_stub_status_module \
 --with-http_sub_module --with-http_v2_module --with-mail --with-mail_ssl_module --with-pcre-jit --with-stream \
 --with-stream_geoip_module --with-stream_realip_module --with-stream_ssl_module --with-stream_ssl_preread_module --with-threads \
 --prefix=. --with-http_ssl_psk && \
make -s -j8
popd
