#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 

COMMON_PACKAGES="gcc cmake pkg-config subversion ghostscript"
UBUNTU_PACKGES="ca-certificates file build-essential ninja-build libusb-1.0-0-dev libcurl4-openssl-dev libcrypto++-dev libcap-dev uuid-dev libavahi-client-dev libssl-dev libsqlite3-dev texlive-font-utils libqt5opengl5-dev qtbase5-dev libzmqpp-dev libboost-system-dev libqrencode-dev python2"
ARCH_PACKAGES="base-devel libusb crypto++ libcap protobuf git sqlite qrencode"
CENTOS_PACKAGES="gcc gcc-c++ ninja-build libcap-devel grpc-devel grpc-plugins protobuf-devel protobuf-compiler libusb-devel cryptopp-devel libcurl-devel libuuid-devel libsqlite3x-devel plantuml avahi-devel qt5-qtbase-devel rpm-build gtest-devel gmock-devel"

COMMON_TOOLS="git doxygen graphviz astyle "

SUDO=sudo
MAKETHREADS=`expr \`grep -c ^processor /proc/cpuinfo\` / 2 + 1`
MAKE="make -s -j${MAKETHREADS}"

PROTOBUF_VERSION=3.11.4
GRPC_VERSION=1.27.3

pushd "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPTDIR=`pwd`

function Warning()
{
    echo "+-+-+-+- Warning +-+-+-+-"
    echo "This script will automatically build and install packages on this system."
    read -p "Are you sure you wish to continue (y/n)?" answer
    case ${answer:0:1} in
        y|Y )
            return 0
        ;;
        * )
            return 1
        ;;
    esac
}

function Prep()
{
    if [ "$UID" == "0" ]; then
        echo "DANGER: running as root!"
        # sudo might not exist, we dont need it anyway
        SUDO=""
    fi

    if ! which lsb_release 2>/dev/null; then
        if which apt 2>/dev/null; then
            ${SUDO} apt update && ${SUDO} apt install -qy --no-install-recommends lsb-release || exit 101
        elif which pacman 2>/dev/null; then
            ${SUDO} pacman -Sy --needed --noconfirm lsb-release || exit 102
        elif which yum 2>/dev/null; then
            ${SUDO} yum install -yq redhat-lsb-core || exit 103
        else
            echo "Can't work out your package manager - install 'lsb_release'"
            exit 100
        fi
    fi
}

function buildProtobuf() {

    if ! pkg-config "protobuf >= $PROTOBUF_VERSION" ; then
    	DEB_ARCH=`dpkg --print-architecture`
    	DEB_FILENAME=libprotobuf_${PROTOBUF_VERSION}-1_${DEB_ARCH}.deb

    	if [ -f ${DEB_FILENAME} ]; then
            ${SUDO} dpkg -i ${DEB_FILENAME}
    	else

            # xenial is too old, artful has issues with ssl libs
            #  build grpc from source
            ${PKGMAN_INSTALL} build-essential git automake autoconf libtool curl unzip pkg-config checkinstall || exit 3

            buildtemp=`mktemp -d`
            pushd ${buildtemp}

            git clone --depth 3 --branch v${PROTOBUF_VERSION} https://github.com/protocolbuffers/protobuf.git && \
            pushd protobuf && \
            ./autogen.sh && \
            ./configure && \
            ${MAKE} && \
            ${SUDO} checkinstall -y --pkgname libprotobuf --pkgversion=${PROTOBUF_VERSION} ${MAKE} install && \
            ${SUDO} ldconfig || exit 5
            mv "${DEB_FILENAME}" "${SCRIPTDIR}"
            popd
            popd
            rm -rf ${buildtemp}
        fi
    else
        echo "According to pkg-config, protobuf is already installed"
    fi
}

function buildGrpc() {

    buildProtobuf

    if ! pkg-config "grpc >= $GRPC_VERSION" ; then
    	DEB_ARCH=`dpkg --print-architecture`
    	DEB_FILENAME=libgrpc++1_${GRPC_VERSION}-1_${DEB_ARCH}.deb

    	if [ -f ${DEB_FILENAME} ]; then
            ${SUDO} dpkg -i ${DEB_FILENAME}
    	else

            # xenial is too old, artful has issues with ssl libs
            #  build grpc from source
            ${PKGMAN_INSTALL} build-essential git automake autoconf libtool curl unzip pkg-config checkinstall || exit 3

            buildtemp=`mktemp -d`
            pushd ${buildtemp}

            git clone --depth 3 --branch v${GRPC_VERSION} https://github.com/grpc/grpc.git && \
            pushd grpc && \
            git submodule update --init && \
            LDFLAGS="`pkg-config --libs-only-L \"protobuf >= 3.5.1\"`" \
        CFLAGS="`pkg-config --cflags-only-I  \"protobuf >= 3.5.1\"`" ${MAKE} && \
            ${SUDO} checkinstall --pkgname libgrpc++1 --pkgversion=${GRPC_VERSION} --requires="libprotobuf (>= ${PROTOBUF_VERSION})" -y ${MAKE} install && \
            ${SUDO} ldconfig || exit 5
            mv "${DEB_FILENAME}" "${SCRIPTDIR}"
            popd
            popd
            rm -rf ${buildtemp}
        fi
    else
        echo "According to pkg-config, grpc is already installed"
    fi
}

function setupUbuntu() {
    echo "Setting up for Ubuntu"
    PKGMAN="${SUDO} apt"
    PKGMAN_UPDATE="${PKGMAN} update -qy"
    PKGMAN_UPGRADE="${PKGMAN} upgrade -qy"
    PKGMAN_INSTALL="${PKGMAN} install -qy --no-install-recommends"

    # Minimum build requirement
    ${PKGMAN_UPDATE} && ${PKGMAN_UPGRADE} || exit 2

    ${PKGMAN_INSTALL} ${COMMON_PACKAGES} ${UBUNTU_PACKGES} >/dev/null || exit 2

    if [ ${RELEASE_MAJOR} -lt 19 ]; then
        echo "!!!!!!!!!!! This os version has an incompatible package that must be removed"
        echo "!!!!!!!!!!! Removing qt5-gtk-platformtheme!"
        ${PKGMAN} remove -q -y qt5-gtk-platformtheme
        buildGrpc
    else
    ${PKGMAN_INSTALL} libprotobuf-dev libgrpc++-dev libssl-dev protobuf-compiler protobuf-compiler-grpc checkinstall || exit 6
    fi

    if [ "${MINIMAL}" == "false" ]; then
        echo Installing extra tools
        # Tools
        ${PKGMAN_INSTALL} ${COMMON_TOOLS} \
            plantuml texlive-latex-base || exit 7

        # Optional libraries
        ${PKGMAN_INSTALL}  \
            opencl-headers || exit 8
        # QT
        ${PKGMAN_INSTALL}  \
            qtbase5-dev || exit 7
        # Testing

        if [ ${RELEASE_MAJOR} -gt 18 ]; then
            ${PKGMAN_INSTALL}  \
                libgtest-dev libgmock-dev || exit 9
        elif [ ${RELEASE_MAJOR} -gt 16 ]; then
            ${PKGMAN_INSTALL}  \
                libgtest-dev google-mock || exit 9
        fi
    else
        echo Skipping extra tools
    fi
}


function AurBuild()
{
    for pkgName in "$@"; do

        if ! pacman -Qi ${pkgName} >/dev/null; then
            if [ "$UID" == "0" ]; then
                    echo "Cant run makepkg as root!"
            else
                buildtemp=`mktemp -d`
                pushd ${buildtemp}

                git clone https://aur.archlinux.org/${pkgName}.git || exit 11
                pushd ${pkgName}

                makepkg -srci || exit 12

                popd
                popd
                rm -rf ${buildtemp}
            fi
        fi
    done
}

function setupArch() {
    echo Setting up Arch linux
    PKGMAN="${SUDO} pacman"
    PKGMAN_UPDATE="${PKGMAN} -Su"
    PKGMAN_UPGRADE="${PKGMAN} -Sy"
    PKGMAN_INSTALL="${PKGMAN} -S --needed --noconfirm"
    export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
    
    ${PKGMAN_UPDATE} && ${PKGMAN_UPGRADE} || exit 2

    ${PKGMAN_INSTALL} ${COMMON_PACKAGES} ${ARCH_PACKAGES}  || exit 2

    if [ "${MINIMAL}" == "false" ]; then
        # Tools
        ${PKGMAN_INSTALL} ${COMMON_TOOLS} || exit 3
        AurBuild plantuml


        # Optional Libraries
        ${PKGMAN_INSTALL} \
            opencl-headers || exit 4
        # QT
        ${PKGMAN_INSTALL} \
            qt5-base
        # Testing
        ${PKGMAN_INSTALL} \
            gtest gmock
    else
        echo Skipping extra tools
    fi
}

function setupCentOS() {
    echo Setting up CentOS linux
    PKGMAN="${SUDO} yum"
    PKGMAN_UPDATE="${PKGMAN} update -qy "
    PKGMAN_INSTALL="${PKGMAN} install -qy "

    ${PKGMAN_UPDATE} && ${PKGMAN_UPGRADE} || exit 2
    ${PKGMAN_INSTALL} ${COMMON_TOOLS} ${COMMON_PACKAGES} ${CENTOS_PACKAGES} || exit 2
}
#####################################
# Main
#####################################

MINIMAL=false
SKIPWARN=false

while getopts ":ymrhd:" opt; do
  case $opt in
  	y)
      echo "Skipping warning" >&2
      SKIPWARN=true
      ;;
  	d)
      echo "Forcing distrobution to $OPTARG" >&2
      DISTRO=$OPTARG
      ;;
    m)
      echo "Minimal install" >&2
      MINIMAL=true
      ;;
    h)
      echo 'usage:'
      echo '   bash '$0' [-m] [-r] [-d <distro>] [-y] 2>&1 | tee setup.log'
      echo ''
      echo ' -m    Minimal install, skip some tools'
      echo ' -d    Force distrobution to be: Ubuntu or Arch'
      echo ' -y    Skip warning and run - careful!'

      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

if [ "$SKIPWARN" == "false" ]; then
	Warning || exit 0
fi

Prep || exit 1

DISTRO=`lsb_release -i | awk '{print($3)}'`
RELEASE=`lsb_release -r | awk '{print($2)}'`
IFS='.' read -ra RELEASE_MAJOR <<< "$RELEASE"

echo Running on ${DISTRO}...
case ${DISTRO} in
    Ubuntu)
        setupUbuntu
    ;;
    Arch)
        setupArch
    ;;
    Antergos)
        setupArch   
    ;;
    CentOS|Fedora)
        setupCentOS
    ;;
    *)
        echo Unknown distrobution
        exit 1
    ;;
esac

echo
echo
echo "Setup completed successfully"
echo '============================'
echo

popd
