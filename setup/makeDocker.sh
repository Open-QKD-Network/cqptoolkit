#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 

VERSION=3.7.3
PUSH=false
RUNTIME=false
REPO=richardcollinsbris
NAME=cqptoolkitbuild
NAME_CUST=false
OPTS=
SUDO=sudo

if [ "$UID" == "0" ]; then
  SUDO=""
fi

pushd "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

while getopts ":phur:v:n:" opt; do
  case $opt in
  	p)
      PUSH=true
      ;;
    r)
      REPO="${OPTARG}"
      ;;
    v)
      VERSION="${OPTARG}"
      ;;
    n)
      NAME="${OPTARG}"
      NAME_CUST=true
      ;;
    u)
      RUNTIME=true
      ;;
    h)
      echo 'usage:'
      echo '   bash '$0' [-p] [-v VER] [-r <repo>] [-n name]'
      echo ''
      echo ' -p    Push the image to <repo>'
      echo ' -r    Name of repository'
      echo ' -v    Image version'
      echo ' -n    Image Name'
      echo ' -u    rUntime version'

      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

OPTS="--build-arg version=$VERSION"

if [ "${RUNTIME}" == "true" ]; then
    echo "Runtime mode"
    if [ "${NAME_CUST}" != "true" ]; then
        NAME=cqp/cqpruntime
    fi
    OPTS="$OPTS --build-arg opts=-r"
fi

${SUDO} docker build $OPTS -t ${REPO}/${NAME}:${VERSION} . && \
${SUDO} docker tag ${REPO}/${NAME}:${VERSION} ${REPO}/${NAME}:latest || exit 1

if [ "${PUSH}" == "true" ]; then
        ${SUDO} docker push ${REPO}/${NAME}:${VERSION} && \
	${SUDO} docker push ${REPO}/${NAME}:latest || exit 2
fi

popd
