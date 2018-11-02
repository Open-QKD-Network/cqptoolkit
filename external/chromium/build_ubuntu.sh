#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
#
if [ -z "$CHROMIUM_VERSION" ]; then
    CHROMIUM_VERSION=70.0.3538.77
fi

apt source chromium-browser && \
cd chromium-browser-${CHROMIUM_VERSION} && \
quilt import -P cqptoolkit-psk-deb.patch ../cqptoolkit-psk-deb.patch ; \
sed -i -e 's/optimize_webui=false$/optimize_webui=false \\\n\tuse_psk=true/' debian/rules && \
dpkg-buildpackage --no-sign -nc

