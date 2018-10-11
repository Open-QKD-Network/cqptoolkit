#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 
if [ -z "$CHROMIUM_VERSION"]; then
    CHROMIUM_VERSION=68.0.3440.106
fi

svn co svn://svn.archlinux.org/packages/chromium && \
cp cqptoolkit_psk.patch chromium/trunk/ && \
cd chromium/trunk && \
source PKGBUILD && sudo pacman -Syu --noconfirm --needed --asdeps "${makedepends[@]}" "${depends[@]}" && \
sed -i -e 's/chromium-skia-harmony.patch)$/chromium-skia-harmony.patch\n\tcqptoolkit_psk.patch)/' PKGBUILD && \
sed -i -e "s/'feca54ab09ac0fc9d0626770a6b899a6ac5a12173c7d0c1005bc3964ec83e7b3')$/'feca54ab09ac0fc9d0626770a6b899a6ac5a12173c7d0c1005bc3964ec83e7b3'\n\tSKIP)/" PKGBUILD && \
makepkg

