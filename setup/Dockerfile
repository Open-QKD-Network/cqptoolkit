# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins
# 
FROM ubuntu:bionic

ARG opts=""
ARG version="3.5"
LABEL version=$version
LABEL description="Build environment for the CQPToolkit. \
Build using:\
sudo ./makeDocker.sh"

ADD ignoreme *.deb /
ADD setupBuild.sh /

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -yq --no-install-recommends lsb-release
RUN chmod +x /setupBuild.sh
RUN /setupBuild.sh -y $opts
