CQP Tool kit {#mainpage}
============

This file is written in [markdown](https://en.wikipedia.org/wiki/Markdown), it can be viewed with the Markdown Viewer chrome extension. The generated documentation can be found [on gitlab](https://qcomms.gitlab.io/cqptoolkit/).

Binary packages can be installed on Ubuntu 18.04+ from [here](https://gitlab.com/QComms/cqptoolkit/-/jobs/artifacts/master/download?job=package%3Adeb).

The system provides various components for integrating [QKD](https://en.wikipedia.org/wiki/Quantum_key_distribution) into a security system.

- Fully documented code using Doxygen
- Written in modern ISO standardised C++11
- The communication uses [GRPC][] which has built in support for [TLS][].
- Generic statistic collection and reporting using the cqp::remote::IReporting interface
- Compatibility drivers for IDQ Clavis 2
- Site Agent
    + Control QKD devices to exchange key
    + Provide pre-shared key on a standard Interface: cqp::remote::IKey
        * Keys can be restricted for use by specific users
    + Creation of indirect key based on XOR'ing key from other sites
    + Configuration through config file, command line arguments or network interface.
    + Automatic Site Agent discovery using [Zeroconf][]
- Network Manager
    + Static/Dynamic control of site agents to create keys between sites based on rules
- Tunnel Controller
    + Uses the IKey interface to get shared keys.
    + Setup of encryption tunnels using 
        * TCP/UDP socket
        * TUN/TAP device (aka VPN)
        * Dedicated physical interface
    + Configuration through config file, command line arguments or network interface.
    + Automatic Site Agent and Tunnel Controller discovery using [Zeroconf][]


This project is intended to be used for both scientific research work and to form part of completed projects. As such it incorporates some approaches which are intended to maintain a production quality level of code and design. More details on the project can be found in [this paper](paper.md).
The build system is based on [CMake][], it is used to produce many different build files from one set of definitions. In order to build the code, one of the build systems, such as gcc, is setup by calling the command `cmake` first. See the build folder for ready made scripts for common environments.
See the [walk through](Walkthrough.md) for details of how the system operates. The [TLS](TLS.md) document details how this system can be used with standard internet communications.

To contribute to this project please see the [Contribution](Contribution.md) file.
## Installation

### Ubuntu 18.04+

Download the packages from [Gitlab](https://gitlab.com/QComms/cqptoolkit/-/jobs/artifacts/master/download?job=package%3Adeb). Extract the zip file and install the tools with ``sudo dpkg -i setup/*.deb build/gcc/CQP-*-Linux-{Algorithms,Networking,CQPToolkit,KeyManagement,QKDInterfaces,CQPUI,Simulate,Tools,UI}.deb``. This will complain about missing dependencies but don't worry, run `sudo apt update && sudo apt install -fy` to install the dependencies and finish the setup.

To install the development files (headers and static libraries) run ``sudo dpkg -i build/gcc/CQP-*-Linux-*-dev.deb``.

For instructions on running the demos see [Demos](DemoHowto.md)

## Exploring the Library

The in-source documentation has details and examples to help developers, ether build the `doc` target (see below) of visit the [CQPToolkit Documentation](https://qcomms.gitlab.io/cqptoolkit/).

The focal point, in terms of QKD, is the `cqp::IKeyPublisher` interface. This provides an outward facing callback called `cqp::IKeyCallback` which can be implemented by any class which wants to receive ready to use key bytes. This is common to both sides of the connection. These interfaces form part of a standard communication mechanism, implemented by the `cqp::SessionController` classes which can be specialised to provide new features.

### Complete programs

There are a number of complete programs which use the library:

| Program          | Description | Interfaces |
| ---------------- | ----------- | ---------- |
| SiteAgentRunner  | Starts a cqp::SiteAgent to control the generation of key using a number of devices | cqp::remote::IKey cqp::remote::ISiteAgent cqp::remote::ISession cqp::remote::INetworkManager cqp::remote::IReporting |
| IDQWrapper       | Acts as a bridge between the CQPToolkit and the IDQuantique Clavis programs, allowing multiple devices to be connected to one system. It is designed to be used from within a docker container | cqp::remote::IIDQWrapper |
| IDQKeyExtraction | Utility to manually get keys from the IDQWrapper | cqp::remote::IIDQWrapper |
| QTunnelServer    | An example of the key sharing interfaces being used. Manages Encryption tunnels, making use of keys shared by SiteAgents | cqp::remote::tunnels::ITunnelServer cqp::remote::IKey cqp::remote::ITransfer |
| SiteAgentCtl     | Sends commands to site agents to start/stop key exchange manually | cqp::remote::ISiteAgent |
| NetworkManager   | Takes information about the network and controls known site agents to generate key in the most efficient way | cqp::remote::ISiteAgent |
| StatsDump        | Prints the statistics generated by a service as csv | cqp::remote::IReporting |
| KeyViewer        | Simple GUI to request and display keys from a cqp::SiteAgent | cqp::remote::IKey |
| QKDTunnel        | Configuration file reader/generator for QTunnelServer | |


### Interfaces
There are two kinds of interfaces, all interface classes are prefixed with an 'I':
1. Internal C++ interfaces used when linking to the library directly. these are under src/CQPToolkit/Interfaces
2. External [Grpc][] interfaces which are compatible with many languages and platforms. These are in the `cqp::remote` namespace. They are defined by .proto files in `src/QKDInterfaces` which are turned into C++ code when the build runs

More details on specific implementations can be found in:
* [Tunnels](Tunnels.md)

## Building
These instructions assume using a 64bit OS to build for 64bit OSs, alternate libraries will need to be installed for 32bit builds.
If you only need to make use of the library rather than change it, it is advised that you use the pre-built binaries from [Gitlab](https://gitlab.com/QComms/cqptoolkit).

The build uses [CMake][] to produce makefiles/solutions/etc for many different platforms and is invoked from an empty build folder which will contain all the output files. Debug builds with produce packages postfixed with a "D".
The build can be controlled by passing options to cmake, eg `-DBUILD_TESTING=OFF`. Run cmake with the `-LH` options to list available switches.

The build structure:

- CMakeLists.txt                     - "Super" project containing all other projects.
    + src
        + CMakeLists.txt          - CQP Project cmake file.
- tests
    + CMakeLists.txt        - Google Test Project for unit test.
        + Interfaces
            + CMakeLists.txt     - Tests of interfaces.

### Linux

#### Docker build environment

Run:

```bash
setup/buildWithDocker.sh
```

The built files will be placed in `build/gcc`.
This uses a docker file from the [gitlab registry](registry.gitlab.com/qcomms/cqptoolkit/buildenv) which can be built manually by running `setup/makeDocker.sh`.

#### Manually

If you don't want to use the ubuntu docker image - or if your building for a system other than Ubuntu 18.04+ or Arch Linux, install the dependencies with:
```bash
./setup/setupBuild.sh
```

Start the debug build from the `build/gcc` or any empty folder by running:
```bash
cmake ../.. && make -j package
```
For a release build, set the `CMAKE_BUILD_TYPE` variable  to ether `Debug`, `Release` or `RELWITHDEBINFO` when calling cmake:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ../.. && make -j package
```

> **Note about protobuf + QT on unbuntu**
> The library `qt5-gtk-platformtheme` is linked against an old version of protobuf which will prevent our QT programs from running.
> This optional dependency can be removed with `apt-get remove qt5-gtk-platformtheme`

### Windows

This setup has be tested on Window 10 and Windows Server 2016 with Visual studio 15 (2016).
The following configurations are supported

| OS        | VS 2017   | QT Creator    | Codeblocks    |
|:----------|-----------|---------------|---------------|
| Linux     |           | gcc           | gcc           |
| Windows   | MSVC      | MSVC / MSYS2  | MSYS2-Mingw   |

#### Required Dependencies
- [CMake][]
- [Tortoise SVN][] and/or [Git for Windows][]

#### Optional Dependencies
- [Doxygen][]
- [GoogleTest Runner for visual studio][]

#### IDE
- [Visual Studio 2016][]
    + Run the [SetupMSBuild.bat](./build/vs2017_x64/SetupMSBuild.bat) script
    + Open the [cqp.sln](./build/vs2017_x64/cqp.sln) solution
    + Select Build->Solution
- [QT Creator for windows][]
    + QT Creator can use ether the native Microsoft compiler or MSYS2. Install MSYS2 separately, you don't need to install the minGW compiler which comes with the QTCreator installer. Or install the [Windows 10 SDK][]
    + Under the component menu, select the components for the compiler your using. E.g msvc2017
    + Using "Open Project", sleect the `CMakeLists.txt` file at the base of the source tree.
- [MSYS2][] and [Codeblocks][]
    + Install the [MSYS2][] package.
    + Run the [installMSYS2Dependencies.bat](build/installMSYS2Dependencies.bat) script
    + Under Settings->Debugger  -> "Create config" called "MSYS2 GDB"
        - Change the debugger to point to gdb.exe (eg. C:\\msys64\\mingw64\\bin\\gdb.exe)
    + Under Settings->Compiler
        + Copy the GCC compiler settings, call it "GCC - Old"
        + Change the Toolchain installation directory to MSYS2's mingw64 install location (eg. C:\\msys64\\   mingw64)
        + In each of the fields under "Program Files" except the "make" program, remove the `mingw32-` prefix.
        + In the make field, add the -j switch to allow multi core building: `mingw32-make.exe -j`
        + Select the previously created "MSYS2 GDB" debugger
        + If you prefer readable output from the compiler,  "Other Settings"->"Compiler Logging" to "Task   description"
    + In the CodeBlock build directory (\\build\\CodeBlocks)
    + Run the `SetupCodeBlocks-MSYS2.bat` file
    + Open the "CQP.cbp" CodeBlocks project file.

If your project file has deep folder structure (this is a bug), it can be improved by right clicking on the workspace and:

- Unchecking "Display folders as on disk"
- Checking "Hide Folder Name"

## Running

The system as many facets but a good starting point is `QKDSim` which will perform all the handshaking and session setup and exchange key by simulating the QKD processes. All tools should show a help message when `-h` is passed as an argument.
The simulator runs as a client and server, start one instance then connect the second to the first. 

```bash
QKDSim -p 8000
```

Shows:
```
INFO: Main Basic application to simulate key exchange
INFO: CreateDevice Device dummyqkd ready
INFO: CreateDevice Device dummyqkd ready
INFO: SiteAgent My address is: mypc:8000
```

This shows that two simulated devices were created by default (one alice and one bob). Now run the second, connecting it to the first:

```bash
QKDSim -r localhost:8000
```

This shows:

```
INFO: Main Basic application to simulate key exchange
INFO: CreateDevice Device dummyqkd ready
INFO: CreateDevice Device dummyqkd ready
INFO: SiteAgent My address is: mypc:38689
INFO: StartServer Listening on mypc:39365
INFO: Connect Waiting for connection from mypc:42245...
INFO: Connect Connected.
INFO: StartNode Node setup complete
```

And the two start to create key, which is made available through the `cqp::IKey` gRPC interface. The `KeyViewer` tool can be used to request keys from this interface.

#### To create a release
- [.Net 3.5][] and [Wix Toolkit][]

### IDE

The recommended IDE for Linux is QT Creator. Open the CMakeLists.txt at the root of the source tree. Use the project options to configure what is built.

Install from the [app centre](apt://qtcreator) or
```
sudo apt-get install qtcreator
sudo apt-get remove qt5-gtk-platformtheme
```

## Serial
For serial devices using the FTDI chip on windows you must install the [FTDI Driver][]

[Doxygen][] For generating the documentation
[GraphViz][] For generating UML diagrams in the documentation and cmake build-in graphs

## OpenCL

This depends on the hardware on the build system:
 - [NVidia - CUDA toolkit][]
 - [Intel SDK][]
 - [AMD OpenCL SDK][]

# Producing a release
## Windows
The official release is made with MSBuild as above. Starting with a clean checkout of the trunk.
1. Ensure that BUILD_VERSION_MAJOR and BUILD_VERSION_MINOR have been set correctly in the toplevel ```CMakeLists.txt```
2. Ensure that [GUID]s have been changed if necessary in ```CMakeModules\\Packaging.cmake```
3. Run ```Build\\Release\\BuildRelease.bat```
4. Copy the following to the release folder:
    - CQP_Build_Env-*.exe
    - CQP-*.msi
    - CQP-*.zip
    - doc.7z

## Linux

Build the `package` target. Releases are built automatically by gitlab based on commands defined in the `.gitlab-ci.yml` file.

## Citing this software

```
@Manual{,
title = {CQPToolkit: A QKD toolkit library},
author = {{Richard Collins, University of Bristol, UK}},
organization = {University of Bristol},
address = {Bristol, UK},
year = 2018,
url = {https://gitlab.com/QComms}
} 
```

## Further Reading
For more details on writing code for the tool kit see [Coding Guide](Coding.md)
Questions and issues see the [FAQ](FAQ.md)

[//]: ##Footnotes

[NVidia - CUDA toolkit]: https://developer.nvidia.com/cuda-downloads
[Intel SDK]: https://software.intel.com/en-us/intel-opencl/download
[AMD OpenCL SDK]: http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/
[Tortoise SVN]: https://tortoisesvn.net/downloads.html
[MSYS2]: https://sourceforge.net/projects/msys2/
[Doxygen]: http://www.stack.nl/~dimitri/doxygen/download.html#srcbin
[GraphViz]: http://www.graphviz.org/Download_windows.php
[CMake]: https://cmake.org/download/
[Enterprise Archetect]: http://www.sparxsystems.com/products/ea/
[FTDI Driver]: http://www.ftdichip.com/Drivers/D2XX.htm
[Wix Toolkit]: http://wixtoolset.org/
[CodeBlocks]: http://www.codeblocks.org/
[GUID]: https://en.wikipedia.org/wiki/Globally_unique_identifier
[Git for Windows]: https://git-scm.com/download/win
[.Net 3.5]: https://www.microsoft.com/en-gb/download/details.aspx?id=22
[Visual Studio 2016]: https://www.visualstudio.com/downloads/
[QT Creator for windows]: https://info.qt.io/download-qt-for-application-development
[GoogleTest Runner for visual studio]: https://marketplace.visualstudio.com/items?itemName=ChristianSoltenborn.GoogleTestAdapter
[Windows 10 SDK]: https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk
[docker file]: https://www.digitalocean.com/community/tutorials/docker-explained-using-dockerfiles-to-automate-building-of-images
[Grpc]: https://grpc.io/docs/
[TLS]: https://en.wikipedia.org/wiki/Transport_Layer_Security
[Zeroconf]: https://en.wikipedia.org/wiki/Zero-configuration_networking
