#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
if [ -z "$CHROMIUM_VERSION" ]; then
    CHROMIUM_VERSION=68.0.3440.106
fi

apt source chromium-browser && \
cp cqptoolkit_psk.patch chromium-browser-${CHROMIUM_VERSION}/debian/patches && \
echo "cqptoolkit_psk.patch" >> chromium-browser-${CHROMIUM_VERSION}/debian/patches/series && \
sed -i -e 's/optimize_webui=false$/optimize_webui=false \\\n\tuse_psk=true \\\n\tuse_custom_libcxx=false/' chromium-browser-${CHROMIUM_VERSION}/debian/rules && \
cd chromium-browser-${CHROMIUM_VERSION} && \
find . -type f -name "*.py" -exec sed -i 's/-stdlib=libc++/-stdlib=libstdc++/' {} + && \
dpkg-buildpackage --no-sign -b -nc

