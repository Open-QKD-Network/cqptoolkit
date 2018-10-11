# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins
# 
FROM ubuntu:bionic

ADD https://developers.yubico.com/YubiHSM2/Releases/yubihsm2-sdk-1.0.4-ubuntu1804-amd64.tar.gz /yubi/
RUN apt update -qy && apt upgrade -qy
RUN apt install -y byobu usbutils opensc nano
RUN cd /yubi && \
    tar -xf yubihsm2-sdk-1.0.4-ubuntu1804-amd64.tar.gz && \
    cd yubihsm2-sdk && \
    dpkg -i --force-depends *.deb && \
    apt install -fy
RUN echo "connector = http://127.0.0.1:12345" > / yubihsm_pkcs11.conf

ENTRYPOINT /usr/bin/byobu


