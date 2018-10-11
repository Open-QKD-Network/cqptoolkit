# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins
# 
FROM ubuntu:bionic

ARG version="0.1"

LABEL version=$version
LABEL description="Environment for running the IDQSequence program.\
This assumes that there is a swarm network available \
Build using:\
sudo ./builddocker.sh"

ADD alice.bit bob.bit eeprom_alice.xml eeprom_bob.xml id3100_Alice.hex id3100_Bob.hex QKDMenu QKDSequence README.txt /id3100/
ADD vectis.conf /var/idq/
ADD *.deb /install/
ADD IDQWrapper /

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt upgrade -qy
RUN apt install -qy --no-install-recommends libcurl4 libusb-1.0-0 libmagic1
RUN dpkg -i /install/*.deb
# For debugging
RUN apt install -yq --no-install-recommends netcat iproute2 iputils-ping net-tools

EXPOSE 7000
ENTRYPOINT ["/IDQWrapper", "-v"]
