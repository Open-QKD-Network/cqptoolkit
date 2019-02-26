# CQP Toolkit FAQ

## Help. Where do I start

### Linux
- Use your package manager to install the QT Creator IDE
- Open QT Creator, select "open a project"
- Select the CMakeLists.txt file at the project root (work copy folder)
- Wait for it to build
- Code!

### Windows
- install Visual studio or QT Creator IDE (see Linux)

## Development

### I've tried to run my program and it fails to run because of a missing dll (eg libstdc++-6.dll)

The runtime dependencies need to be ether in the current folder or in the PATH variable. For MinGW, ensure that the mingw/bin folder is in the PATH when running the program. For installed runtimes, add the file to the ***CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS*** variable in ***CMakeFileLists.txt***

### ZeroMQ gives me an error about non-existent path

```
CMake Error in src/Tools/Clavis3Driver/CMakeLists.txt:
  Imported target "PkgConfig::ZeroMQ" includes non-existent path

    "/usr/lib/pgm-5.2/include"
```
This is an issue with the pkgconfig of openpgm, edit the file: `/usr/lib/pkgconfig/openpgm-5.2.pc`
remove the `-I${libdir}/pgm-5.2/include` from the line begining `Cflags`. Then send an email to the openpgm developers, asking them to fix this issue.

## Runtime

### Trying to run a gui program crashes with something about protobuf versions

The full error:

> `[libprotobuf FATAL google/protobuf/stubs/common.cc:79] This program was compiled against version 3.0.0 of the Protocol Buffer runtime library, which is not compatible with the installed version (3.5.1).  Contact the program author for an update.  If you compiled the program yourself, make sure that your headers are from the same version of Protocol Buffers as your link-time library.  (Version verification failed in "/build/mir-y44crS/mir-0.28.0+17.10.20171011.1/obj-x86_64-linux-gnu/src/protobuf/mir_protobuf.pb.cc".)`
> `terminate called after throwing an instance of 'google::protobuf::FatalException'`

This is due to `qt5-gtk-platformtheme` being installed
Remove it with `apt remove qt5-gtk-platformtheme`
