CQP Tool kit {#mainpage}
============

[//]: # "This file is written in markdown and rendered with [Doxygen][], the rendered documentation can be found on the gitlab project cqptoolkit."

The system provides various components for integrating [QKD](https://en.wikipedia.org/wiki/Quantum_key_distribution) into a security system. It's written in C++11 but uses [GRPC][] interfaces so can be integrated with lots of different languages.

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

```bash
git clone https://gitlab.com/QComms/cqptoolkit.git
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

Below is a flow chart to help find the area relevant to you as the project covers many different aspects of QKD and key management - contributors are welcome to drive this project to be more specialised.
QKD requires some form of [non-cloning](https://en.wikipedia.org/wiki/No-cloning_theorem) communication, usually by using single photons over a fibre optic cable. They can operate point-to-point or as one-to-many but they inherently have a physical location (where the fibre terminates) - they cannot be virtualised! The point at which the photon is transmitted or detected is the boundary of the secure system - almost like the [firewall](https://en.wikipedia.org/wiki/Firewall_(computing)) to a network. Once the in-divisible photons have been turned into a string of bits to form a [symmetric key](https://en.wikipedia.org/wiki/Key_(cryptography)) the standard rules of computer security like authentication, access control, etc, apply. The difference is that once those keys have been produced, each of the QKD devices have a number which [no one else knows](https://en.wikipedia.org/wiki/Shared_secret) [proven by science](https://arxiv.org/pdf/quant-ph/0003004.pdf).

The nature of this "firewall" effect is that the systems controlling the QKD devices need to be secure and considered trusted - also called "trusted node" - where and how you draw the line can range from armed guards to just [locking the server room door](https://www.youtube.com/watch?v=rnmcRTnTNC8).

The in-source documentation has more details and examples to help developers, you can see them online [here](https://qcomms.gitlab.io/cqptoolkit/), they can also be built by the `doc` target.

    @startuml
    title Where to start \n

    skinparam activity {
      StartColor #EF476F
      BarColor #FFD166
      EndColor #EF476F
      BackgroundColor #06D6A0
      BorderColor #118AB2
      FontName Impact
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
        Keys can be obtained by using the [[./Index.html#IKeyInterface]] interface;

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
