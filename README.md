CQP Tool kit
============

[//]: # "This file is written in markdown and rendered with [Doxygen][], the rendered documentation can be found on the gitlab project cqptoolkit."

The system provides various components for integrating [QKD](https://en.wikipedia.org/wiki/Quantum_key_distribution) into a security system. It's written in C++11 but uses [GRPC][] interfaces so can be integrated with lots of different languages.

# Quick Start

To run the software natively, ether:

- download and install the [Ubuntu deb packages](https://gitlab.com/QComms/cqptoolkit/-/jobs/artifacts/master/download?job=package%3Adeb) or 
- clone the source, ensuring submodules are updated and build locally

To clone the source including submodules:
```bash
git clone --recurse-submodules https://github.com/Open-QKD-Network/cqptoolkit.git
```

> If you cloned without using `--recurse-submodules`, submodules can be updated by running `git submodule update --init` from within the source folder.

```bash
sudo apt install pkg-config ca-certificates file build-essential cmake ninja-build libusb-1.0-0-dev libcurl4-openssl-dev \
	libcrypto++-dev libcap-dev uuid-dev libssl-dev libsqlite3-dev libprotobuf-dev libgrpc++-dev \
	libssl-dev protobuf-compiler protobuf-compiler-grpc checkinstall 
mkdir build-cqptoolkit
cd build-cqptoolkit
cmake -G Ninja ../cqptoolkit && ninja
```

If the binaries have been installed, omit the paths to the commands from the instructions.
To run two sites each with a QKD device from the build folder, first start site "A" by starting a site agent and connecting an Alice "dummy driver" to it:

```bash
./src/Tools/SiteAgentRunner/SiteAgentRunner -p 8000 &
./src/Drivers/DummyQKDDriver/DummyQKDDriver -r localhost:8000 -a
```

```bash
./src/Tools/SiteAgentRunner/SiteAgentRunner -p 8001 &
./src/Drivers/DummyQKDDriver/DummyQKDDriver -r localhost:8001 -b
```

This will not start producing key immediately as this system is designed to be controlled by a management system, the connection needs to be established.
Verify the list of available devices with:

```bash
./src/Tools/SiteAgentCtl/SiteAgentCtl -d -c localhost:8000
```

should produce something similar to

```json
{
 "url": "<hostname>:8000",
 "devices": [
  {
   "config": {
    "id": "dummyqkd__0__16_alice",
    "kind": "dummyqkd"
   },
   "controlAddress": "<hostname>:34219"
  }
 ]
}
```

and port `9001` should produce something similar to

```json
{
 "url": "<hostname>:8001",
 "devices": [
  {
   "config": {
    "id": "dummyqkd__0__16_bob",
    "side": "Bob",
    "kind": "dummyqkd"
   },
   "controlAddress": "<hostname>:38367"
  }
 ]
}
```

The connection can now be made by calling:

```bash
./src/Tools/SiteAgentCtl/SiteAgentCtl -c localhost:8000 -j localhost:8001
```

This will create a single hop from one site to the next, more complex routes can be defined by using the `-a` option with a JSON string specifying the path.

After a few seconds there should be key available which can be tested by requesting a key.
> NOTE: The -k parameter must be the url shown in the details of the second site, not "localhost:9001"

```bash
./src/Tools/SiteAgentCtl/SiteAgentCtl -c localhost:8000 -k `hostname`:8001
```

The link can be stopped with the unjoin command:


```bash
./src/Tools/SiteAgentCtl/SiteAgentCtl -c localhost:8000 -u localhost:8001
```

> Not that key is still available even though generation has stopped, as long as the site agents are running.
> It can be requested with the same key request command above.

## Encryption example

Launch the site agents and drivers as above and start the key exchange.

Start one side of the "VPN" on the bob/far side which will listen on port 9010 and connect to Bob's key store:
```
./src/Tools/QTunnelServer/QTunnelServer -p 9010 --keystore-url=`hostname`:8001
```
Now start the Alice side of the VPN, defining the tunnel to create. Two ports will be opened, one for each side on 9000 and 9001, anything entering
these ports will be encrypted, transfered to the other side, decrypted and produced on the other port.
```
./src/Tools/QTunnelServer/QTunnelServer --keystore-url=`hostname`:8000 --remote=localhost:9010 --start-node=tcpsrv://0.0.0.0:9000 --end-node=tcpsrv://0.0.0.0:9001
```

Anything which uses tcp communications can then use this port, netcat is a simple program which will send data over the ports, start one on one side:

```
nc localhost 9000
```

and one on the other:

```
nc localhost 9001
```

Anything typed into one side will appear on the other when enter is pressed.  Inspecting the packets travelling through ports `9000` and `9001`
with a tool such as wireshark will show the data being encrypted and the key ID used.

Other forms of connection can be created, instead of `tcpserv`:

| Example					    | Description 											|
| ============================= | ===================================================== |
| tcpserv://0.0.0.0:1234		| A listening port is created on port 1234 				|
| tcp://127.0.01:1234			| A connection to tcp port 1234 on localhost is made 	|
| udp://0.0.0.0:1234			| UDP packets are sent from this port					|
| tun://192.168.101.1/?netmask=255.255.255.0	| An IP level tunnel device is created with an IP address |
| tap://192.168.101.1/?netmask=255.255.255.0	| An ethernet level tap device is created |
| eth://eth0/?level=tcp         | Create a raw socket, level can be tcp, ip or eth. 	|

# Progress

Planed and completed features

- [ ] Device Drivers
    + [x] Compatibility drivers for IDQ Clavis 2
        - [ ] Device feedback
    + [x] Compatibility drivers for IDQ Clavis 3
    + [x] University of Bristol handheld free space device
    + [ ] University of Bristol on chip device
- [ ] Post processing
    + [x] Alignment for asynchronous systems
    + [x] Sifting for synchronous systems
    + [ ] Error correction
    + [ ] Privacy amplification
- [x] Multi-site key management (Site Agents)
    + [x] Control QKD devices to exchange key
    + [x] Provide pre-shared key on a standard Interface: cqp::remote::IKey
        - [x] Keys can be restricted for use by specific users
    + [x] Creation of indirect key based on XOR'ing key from other sites
    + [x] Configuration through config file, command line arguments or network interface.
    + [x] Automatic Site Agent discovery using [Zeroconf][]
    + [ ] Resolve keys across links between trusted sites (issue #8)
- [x] Network Management of site agents
    + [x] Sites can be controlled through interfaces
    + [ ] Static/Dynamic control of site agents to create keys between sites based on rules
- [x] Encrypted tunnel controller (like stunnel)
    + [x] Uses the IKey interface to get shared keys.
    + [x] Setup of encryption tunnels using
        - [x] TCP/UDP socket
        - [x] TUN/TAP device (aka VPN)
        - [x] Dedicated physical interface
    + [x] Configuration through config file, command line arguments or network interface.
    + [x] Automatic Site Agent and Tunnel Controller discovery using [Zeroconf][]
- [ ] Multiple Platforms
    + [x] Linux
    + [ ] Windows (see issue #2)

It is hoped that this project can prove useful for both scientific research work and large projects.  More details on the project can be found in [this paper](paper.md).

To contribute to this project please see the [Contribution](Contribution.md) file.

## Installation

The system currently works on Linux - Windows is planned for the future.

### Docker image

There is a ready to run docker image in the [gitlab registry][]. You can run with
`sudo docker run -it --rm registry.gitlab.com/qcomms/cqptoolkit/runtime`. Add a command on the end to run something directly, for example to run a simulation of the QKD key generation with `QKDSim`:

```bash
sudo docker run -it --rm registry.gitlab.com/qcomms/cqptoolkit/runtime AlignmentTests
```

### Ubuntu 18.04+

You can install binary packages from [Gitlab](https://gitlab.com/QComms/cqptoolkit/-/jobs/artifacts/master/download?job=package%3Adeb). Extract the zip file and install the tools with `dpkg`, it will complain about missing dependencies but don't worry, the second line will fix them.

```bash
sudo dpkg -i setup/*.deb build/gcc/CQP-*-Linux-{Algorithms,Networking,CQPToolkit,KeyManagement,QKDInterfaces,CQPUI,Simulate,Tools,UI,Drivers,IDQDevices}.deb
sudo apt install -fy
```

To install the development files (headers and static libraries) run `sudo dpkg -i build/gcc/CQP-*-Linux-*-dev.deb ; sudo apt install -fy` instead.

### From source

Building from source requires that you install dependencies listed in the `setup/setupbuild.sh`, currently it works for Ubuntu and Arch Linux.  You can build inside a [docker container](https://www.docker.com/resources/what-container) which already has all the dependencies installed with:

```bash
sudo docker run -it registry.gitlab.com/qcomms/cqptoolkit/buildenv
```

Clone the source from [gitlab](https://gitlab.com/QComms/cqptoolkit.git) with [git][] and build with [CMake][] and [gnu make](https://www.gnu.org/software/make/):
The `--recurse-submodules` adds the optional extras - access to some of these is restricted to UoB and it's partners, the build will work without them.

```bash
git clone --recurse-submodules https://gitlab.com/QComms/cqptoolkit.git
```

If you want to get all submodules, and have the appropreate logins run:
```bash
git submodule update --checkout
```

```bash
mkdir cqptoolkit-build
cd cqptoolkit-build
cmake ../cqptoolkit && nice make -s -j
# this can be installed with
sudo make install
```

The build uses [CMake][] to produce makefiles/solutions/etc for many different platforms and is invoked from an empty build folder which will contain all the output files. Debug builds with produce packages post fixed with a "D".
The build can be controlled by passing options to cmake, Eg `-DBUILD_TESTING=OFF`. Run cmake with the `-LH` options to list available switches.

To make changes and develop the library it's recommended to install [QT Creator](http://doc.qt.io/qtcreator/) and open the project by [selecting the CMakeLists.txt file](https://codeyarns.com/2016/01/26/how-to-import-cmake-project-in-qt-creator/). It is advisable to use parallel builds by going to Projects->Build Steps-> Details and adding `-j<number>` to the tool parameters, see [so](https://stackoverflow.com/questions/8860712/setting-default-make-options-for-qt-creator).
Once built the files, by default, are at the same level as the project folder called `build-<project name>-<platform>-<target>`.

> **Note about protobuf + QT on Ubuntu**
> The library `qt5-gtk-platformtheme` is linked against an old version of protobuf which will prevent our QT programs from running.
> This optional dependency can be removed with `apt-get remove qt5-gtk-platformtheme`

## Exploring the Library

    @startuml
        title Applicaiton Overview

        component "QKD Device Drivers" as drv
        interface IDevice as idev
        drv - idev

        component "Device contol and\n Key storage" as sa
        interface IKey as ikey
        sa - ikey
        sa ..> idev

    package "Key consumers" as kc {
        component "Custom VPN" as tun
        component "Custom\nWeb Server" as nginx
        component "HSM bridge" as hsm
    }
    tun ..> ikey
    nginx ..> ikey
    hsm ..> ikey

    package Utilities {
        component "Config/Control GUI" as gui
        component "Simulators/Testing" as sim
        component "Data extraction" as stats

        gui -[hidden]down- sim
        sim -[hidden]down- stats
    }
    @enduml

Below is a flow chart to help find the area relevant to you as the project covers many different aspects of QKD and key management - contributors are welcome to drive this project to be more specialised.
QKD requires some form of [non-cloning](https://en.wikipedia.org/wiki/No-cloning_theorem) communication, usually by using single photons over a fibre optic cable. They can operate point-to-point or as one-to-many but they inherently have a physical location (where the fibre terminates) - they cannot be virtualised! The point at which the photon is transmitted or detected is the boundary of the secure system - almost like the [firewall](https://en.wikipedia.org/wiki/Firewall_(computing)) to a network. Once the in-divisible photons have been turned into a string of bits to form a [symmetric key](https://en.wikipedia.org/wiki/Key_(cryptography)) the standard rules of computer security like authentication, access control, etc, apply. The difference is that once those keys have been produced, each of the QKD devices have a number which [no one else knows](https://en.wikipedia.org/wiki/Shared_secret) [proven by science](https://arxiv.org/pdf/quant-ph/0003004.pdf).

The nature of this "firewall" effect is that the systems controlling the QKD devices need to be secure and considered trusted - also called "trusted node" - where and how you draw the line can range from armed guards to just [locking the server room door](https://www.youtube.com/watch?v=rnmcRTnTNC8).

If you can't see the diagram below, please go to the [online documentation](https://qcomms.gitlab.io/cqptoolkit/), it can also be built by the `doc` target.

    @startuml
    title Where to start \n

    skinparam activity {
      StartColor #EF476F
      BarColor #FFD166
      EndColor #EF476F
      BackgroundColor #06D6A0
      BorderColor #118AB2
    }

    start
    if (Do you have a QKD Device?) then (Yes)
        if (Does your QKD device have a driver?) then (No)
            :You will need to [[./index.html#CreatingDrivers create a driver]] which implements the
            <b>IDriver</b> and <b>IReporting</b> interfaces.;
            if (Use the library?) then (Yes)
                :See the [[./index.html#RunningDummyQKDDriver DummyQKDDriver]] for an example.;
            else (No)
            endif
        else (Yes)
        endif
    else (No)
        :Check out how to run the
        [[./index.html#RunningDummyQKDDriver DummyQKDDriver]];
    endif

    if (Do you want keys for multiple
    locations in a network?) then (Yes: Sites)
        :[[./index.html#Registering Register your driver]] with the site agent
        using the <b>ISiteAgent</b> Interface
        Keys can be obtained by using the [[./index.html#IKeyInterface IKey]] interface;

        if(Do you want keys to be stored/persist between restarts?) then (Yes)
            if (Do the keys need to be secured?) then (Yes)
                :Use the [[./index.html#HSMs HSM storage]] options;
                if (Is your HSM supported?) then (No)
                    :Create a driver that implements
                    the <b>IBackingStore</b> interface.
                    Add the driver creation to the
                    <b>BackingStoreFactory</b>;
                else (Yes)
                endif
            else (No)
                : When running SiteAgentRunner,
                use the option ""-b file"" or
                set ""backingStoreUrl"" to
                ""file:///filename.db"";
            endif
        else (No)
        endif
    else (No: Point-to-Point)
        :Use the [[./index.html#IDeviceInterface IDevice interface]] to
        control the driver for your system.
        As keys become available they will
        be sent via the call to <b>WaitForSession</b>.;
    endif

    if (Do you want to use the key for anything?) then (Yes)
        :See the [[./index.html#Encryption encryption section]]
        for an example of using the [[./index.html#IKeyInterface IKey interface]];
    else (No)
    endif
    stop

    @enduml
    

### Running DummyQKDDriver <a name="RunningDummyQKDDriver" />

As with most programs, passing `-h` to it will display the options available. The DummyQKDDriver runs a standard set of post processing steps on simulated photon detections using the cqp::DummyQKD class. There needs to be two instances of the program running, one for Alice and one for Bob.

Run Bob first on port 8000 by calling:
```bash
DummyQKDDriver -b -k 0.0.0.0:8000
```
Now run Alice, telling her to connect to Bob and start exchanging key (in manual mode):

```bash
DummyQKDDriver -a -m localhost:8000
```

If it worked you'll get a flood of error message like this: `ERROR: OnKeyGeneration No listener for generated key`. This is because while running the system like this is nice to see it doing something, it's not very useful, there's nowhere to put the key that is generated. the drivers are designed to be used by something.
The project has a system for managing the keys called [Site Agents](#SiteAgents) or you can talk to the drivers directly using the [IDevice interface](#IDeviceInterface)

### Site Agents <a name="SiteAgents" />

Site agents can be run with the SiteAgentRunner, we can run two sites with:
```bash
SiteAgentRunner -p 9000
```
and:
```bash
SiteAgentRunner -p 9001
```
Now we can attach our DummyQKDDriver's, one to each site:
```bash
DummyQKDDriver -a -r localhost:9000 # run as Alice, register with site agent
```
and:
```bash
DummyQKDDriver -b -r localhost:9001 # run as Bob, register with site agent
```
> We don't care which ports the devices use - the sites will work it out themselves.

Nothing will happen yet as the site agents don't know what to do with these devices. We can instruct them to start making key by sending the cqp::ISiteAgent::StartNode command:
```bash
SiteAgentCtl -c localhost:9000 -b '{"hops":[{"first":{"site":"localhost:9000","deviceId":"dummyqkd__0__16_alice"},"second":{"site":"localhost:9001","deviceId":"dummyqkd__0__16_bob"}}]}'
```
This is a long winded way of saying connect A to B but it is very powerful, allowing multiple hops to be specified to produce an end-to-end secure key from a chain of devices.  This JSON string can be specified in the site agents "staticHops" config field to let this happen automatically once all the devices are available.
The link can then be stopped with:
```bash
SiteAgentCtl -c localhost:9000 -e '{"hops":[{"first":{"site":"localhost:9000","deviceId":"dummyqkd__0__16_alice"},"second":{"site":"localhost:9001","deviceId":"dummyqkd__0__16_bob"}}]}'
```
> Note the `-e` instead of `-b` to *end* the link instead of *beginning* it.

More complex setups can be achieved by implementing the cqp::remote::INetworkManager interface yourself to issue the commands. Feedback from the devices comes over the cqp::remote::IReporting interface on the same socket so that you can react to changes in the system. [Here](#Reporting) you can see how to extract things from the reporting interface with the StatsDump tool.

### Creating Drivers <a name="CreatingDrivers" />

The driver application is a bridge between the internal device interfaces (cqp::IQKDDevice) and the external cqp::remote::IDevice interface. The cqp::RemoteQKDDevice class handles most of the work for you, the application must handle the configuration and creation of the device.

The real work is in creating a driver to setup the device and read the key. If your device just produces raw detections then you will need to configure a [processing pipeline](#ProcessingPipelines) like cqp::DummyQKD or cqp::PhotonDetectorMk1 and cqp::LEDAliceMk1. If your device generates ready to use key like the cqp::Clavis3Device you need to read it and publish it over the cqp::IKeyCallback interface (use cqp::KeyPublisher).

Both these approaches require a form of session management, provided by cqp::session::SessionController and cqp::session::AliceSessionController, these implement the cqp::ISessionController and cqp::remote::ISession interface and are used by cqp::RemoteQKDDevice to start and stop the device and it's peer.  These are usually all that's needed but in some situations they need to be specialised to cope with device requirements

    @startuml Readme_Drivers
    title Anatomy of a driver
        package Application {
            namespace cqp #DDDDDD {
                class RemoteQKD
                interface IQKDDevice {
                    GetSessionController()
                }
                class "SessionController" as session
                interface "IDetector::Service" as detServ {
                    StartDetecting()
                    StopDetecting()
                }
                class "Provider<IDetectionEventCallback>" as provider {
                    Attach()
                    Dettach()
                    Emit()
                }

                RemoteQKD .r.> IQKDDevice : uses
                IQKDDevice -r[hidden]-> session
            }
            class Main {
                main()
            }
            class MyDriver
            class Detector

            MyDriver .u.|> cqp.IQKDDevice
            MyDriver o-u-> cqp.session
            Detector .u.|> cqp.detServ
            Detector -u-|> cqp.provider

            Main o-> MyDriver
            MyDriver o-> Detector
            Main o-u-> cqp.RemoteQKD

            note bottom of MyDriver
                In this case the driver is a simple detector
                which produces detection. Post processing detail not shown.
                MyDriver pull together all the parts to run the driver.
            end note

            note bottom of Detector
                The detector controls the device
                and outputs the data using the Provider
            end note
        }
    @enduml

### Registering a driver <a name="Registering" />

At the time of writing, all the drivers can be registered with a site agent by using the `-r` switch. This causes the cqp::RemoteQKDDevice to call cqp::remote::ISiteAgent::RegisterDevice on the cqp::SiteAgent, it will then use the cqp::remote::ISession interface to start/stop the device.

### IDevice Interface <a name="IDeviceInterface" />

This interface allows a more direct access to the device than going through the site agents. Key generated by the device are immediately fed back to the caller. First call cqp::remote::IDevice::WaitForSession, then cqp::remote::IDevice::RunSession. Call cqp::remote::IDevice::EndSession to stop generating key.

### HSMs <a name="HSMs" />

[HSMs](https://en.wikipedia.org/wiki/Hardware_security_module) are storage devices which are physically secure digital vaults. There is a standard interface for them called [PKCS#11](https://en.wikipedia.org/wiki/PKCS_11). Each manufacturer has their own interfaces and support for PKCS#11 is sketchy, however there is a software implementation which we use as a reference called [SoftHSM2](https://www.opendnssec.org/softhsm/). The cqp::keygen::HSMStore class proves an implementation that links the cqp::SiteAgent with the cqp::IBackingStore interface.

### IKey Interface <a name="IKeyInterface" />

Sites provide keys more many endpoints (the available key stores can be retrieved by calling cqp::remote::IKey::GetKeyStores), they can be requested by using the cqp::remote::IKey interface. Once a new key has been requested, the other side can only retrieve it as an existing key - this prevents clashing and misuse of keys.

### Encryption <a name="Encryption" />

More details on specific implementations can be found in:
* [Tunnels](Tunnels.md)

### Reporting <a name="Reporting" />

Changes to values in the system are published externally via the cqp::remote::IReporting interface and internally with the cqp::stats::Stat class.
You can register for all stats by calling cqp::remote::IReporting::GetStatistics with the cqp::remote::ReportingFilter::listIsExclude field set to `true` or specific filters can be specified.

### Processing Pipelines <a name="ProcessingPipelines" />

Standard [BB84 QKD protocol](https://en.wikipedia.org/wiki/BB84) requires post processing steps to turn the raw detections into usable keys.

- **Alignment** Finding the start and end of the real transmission.
    + Adjusting for timing differences/drift etc.
    + Compensating for phase or other differences between the transmitter and receiver, etc
- **Sifting** Throwing away invalid detections
- **Error Correction** Correcting incorrect detections - without divulging the values
- **Privacy amplification** Hashing the key to make any divulged bits useless

An example of a processing pipeline can be seen in cqp::DummyQKD::ProcessingChain.

## Building

The builds are managed by the gitlab [continuous integration system](https://about.gitlab.com/product/continuous-integration/) defined in the `.gitlab-ci.yml` file.

The docker images can by built manually by running `setup/makeDocker.sh`.

### Windows

This is work in progress.
This setup has be tested on Window 10 and Windows Server 2016 with Visual studio 15 (2016).
The following configurations are supported

| OS        | VS 2017   | QT Creator    | Codeblocks    |
|:----------|-----------|---------------|---------------|
| Linux     |           | gcc           | gcc           |
| Windows   | MSVC      | MSVC / MSYS2  | MSYS2-Mingw   |

#### IDE

- [Visual Studio 2016][]
    + Run the [InstallVcPkg.bat](./build/vs2017_x64/InstallVcPkg.bat) script once to install vcpkg in C:\vcpkg
    + Run the [SetupMSBuild.bat](./build/vs2017_x64/SetupMSBuild.bat) script
    + Open the [cqp.sln](./build/vs2017_x64/cqp.sln) solution
    + Select Build->Solution
- [QT Creator for windows][]
    + QT Creator can use ether the native Microsoft compiler or MSYS2. Install MSYS2 separately, you don't need to install the minGW compiler which comes with the QTCreator installer. Or install the [Windows 10 SDK][]
    + Under the component menu, select the components for the compiler your using. E.g msvc2017
    + Using "Open Project", select the `CMakeLists.txt` file at the base of the source tree.
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

[gitlab registry]: https://gitlab.com/QComms/cqptoolkit/container_registry
[MSYS2]: https://sourceforge.net/projects/msys2/
[Doxygen]: http://www.stack.nl/~dimitri/doxygen/download.html#srcbin
[CMake]: https://cmake.org/download/
[CodeBlocks]: http://www.codeblocks.org/
[git]: https://git-scm.com/
[.Net 3.5]: https://www.microsoft.com/en-gb/download/details.aspx?id=22
[Visual Studio 2016]: https://www.visualstudio.com/downloads/
[QT Creator for windows]: https://info.qt.io/download-qt-for-application-development
[GoogleTest Runner for visual studio]: https://marketplace.visualstudio.com/items?itemName=ChristianSoltenborn.GoogleTestAdapter
[Windows 10 SDK]: https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk
[Grpc]: https://grpc.io/docs/
[Zeroconf]: https://en.wikipedia.org/wiki/Zero-configuration_networking
