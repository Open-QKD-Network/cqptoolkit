#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
#

apt source stunnel4 && \
cd `find -maxdepth 1 -type d -name 'stunnel4-*'` && \
quilt import -P cqptoolkit-psk-deb.patch ../cqptoolkit-psk-deb.patch && \
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage --no-sign -j4
