#!/bin/python2
# Copyright (C) University of Bristol 2017
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins
# 
from setuptools import setup
import os
import sys
import subprocess

if sys.version_info[0] == 3:
    # Python 3
    from distutils.command.build_py import build_py_2to3 as _build_py
else:
    # Python 2
    from distutils.command.build_py import build_py as _build_py


def generate_proto(source, require = True):
    """Invokes the Protocol Compiler to generate a _pb2.py from the given
    .proto file.  Does nothing if the output already exists and is newer than
    the input."""

    if not require and not os.path.exists(source):
        return

    output = source.replace(".proto", "_pb2.py").replace("../src/QKDInterfaces/", "")

    if (not os.path.exists(output) or
            (os.path.exists(source) and
                     os.path.getmtime(source) > os.path.getmtime(output))):
        print("Generating %s..." % output)

        if not os.path.exists(source):
            sys.stderr.write("Can't find required file: %s\n" % source)
            sys.exit(-1)

        protoc_command = [ "python", "-m", "grpc_tools.protoc", "-Isrc/QKDInterfaces", "--python_out=src/QuantumSDNAgent/GeneratedClasses/", "--grpc_python_out=src/QuantumSDNAgent/GeneratedClasses/", source ]
        subprocess.check_output(protoc_command)


class build_py(_build_py):

    def run(self):
        # Generate necessary .proto file if it doesn't exist.
        generate_proto("src/QKDInterfaces/Common.proto")
        generate_proto("src/QKDInterfaces/Site.proto")
        generate_proto("src/QKDInterfaces/Keys.proto")
	generate_proto("src/QKDInterfaces/Reporting.proto")
        generate_proto("src/QKDInterfaces/IQuantumPath.proto")
        generate_proto("src/QKDInterfaces/ISiteAgent.proto")
        generate_proto("src/QKDInterfaces/INetworkManager.proto")
        generate_proto("src/QKDInterfaces/IReporting.proto")

        # _build_py is an old-style class, so super() doesn't work.
        _build_py.run(self)


if __name__ == '__main__':

    generated_classes_path = "src/QuantumSDNAgent/GeneratedClasses"
    if not os.path.exists(generated_classes_path):
        print "Creating GeneratedClasses dir.."
        os.makedirs(generated_classes_path)
    filename = '__init__.py'
    with open(os.path.join(generated_classes_path, filename), 'wb') as temp_file:
        temp_file.write("")

    setup(
        name='CQPToolkit',
        version='1.0',
        packages=['src', 'src.NetworkManager', 'src.QuantumSDNAgent', 'src.QuantumSDNAgent.GeneratedClasses',
                  'src.QuantumSDNAgent.QuantumPathAlgorithm'],
        install_requires = ['grpcio', 'grpcio-tools', 'google', 'protobuf', 'futures', 'matlab', 'networkx'],
        url='',
        author='fn17485',
        description='Network Manager setup',
        cmdclass={
            'build_py': build_py
        },
    )
