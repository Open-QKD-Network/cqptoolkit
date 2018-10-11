# Coding Guide
@tableofcontents

## Style
Coding style is an emotive topic in the software community. Built into the build is the ```astyle``` pretty printer, building the ```style``` target will format the code to conform with the [Google C++ Style Guide][] with minor changes which are declared in the ```astyle.options```. End of descussion.

## Interfaces

```cpp
// An interface
class IInterface {
public:
    virtual void DoStuff() = 0;
}
// An class which implements an interface .
class Implementer : public virtual IInterface {
public:
    virtual void DoStuff() override;
}
```

The interfaces in the tool kit are declared as abstract classes with only pure virtual functions[^1] and no member variables. This is to isolate the **What** from the **how**. The interfaces cannot have ```.cpp``` implementation files as they may be implemented in any language or platform. Interface names should be prefixed with ```I```

[C++ Interfaces](http://www.drdobbs.com/cpp/c-interfaces/184410630)

Classes can then implement the any number of interfaces as a promise to fulfil certain roles. The inheritance is both **public** so that others can see that the interface is provided and **virtual** which states that there is no data associated with the inheritance - it is only a contract for *what* the class will do.

Multiple, non-virtual inheritance is usually a bad idea and is a symptom of an overly complex class.

The **Override** keyword should be used when overriding any inherited function, it will alert you to any mismatch in the signature between your function and the function you intended to override.

[More Info on virtual inheritance](http://www.cprogramming.com/tutorial/virtual_inheritance.html)

## Serialisation

## Threads

## Concurrency

When using multiple threads, care must be taken when accessing variables. C++ provides the basics to handle most situations.

### Multiple threads accessing a member/static variable
```cpp
#include <mutex>

class a {
public:
    SetX(int n) {
        std::lock_guard<std::mutex> lock(myvar_lock);
        // From hear until the lock variable goes out of scope we can safely access myvar
        myvar = n;
    }
private:
    int myvar;
    // a lock to protect myvar
    std::mutex myvar_lock;
};
```
### Efficient blocking on state

If a certain state must exist before a program must continue, use the condition variable

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>

class a {
public:
    int Pop()
    {
        std::unique_lock<std::mutex> lock(myvar_lock);
        myvar_cv.wait(
            lock,   // mutex to use for locking
            []{     // lambda statement to return descition as to wether to continue
                return myvar.size() > 0;
            }
        );

        // from now on we own the lock on myvar
        int result = myvar.pop_front();
        // this could also be done my restricting the scope of myvar_lock
        // its best to unlock the mutex before notifying others
    }

    void Push(int n)
    {
        {
            std::unique_lock<std::mutex> lock(myvar_lock);
            
            myvar.push_back(n);
        }

        // this could also be done by calling lock.unlock();
        // its best to unlock the mutex before notifying others
        myvar_cv.notify_one();
    }
    
private:
    queue<int> myvar;
    // a lock to protect myvar
    std::mutex myvar_lock;
    // variable to wait on for changes
    std::condition_variable myvar_cv;
};
```

## Testing

An effective approach to writing software is to start with tests which perform the steps you require, at firt these steps will fail, the code is then written to fulfill the requirements of the test. The code can then be written very efficiently and makes you think about what interface/requirements your class will have.
When creating the QTunnel classes, the following test was first created to outline what the class should do:

```cpp
TEST_F(TestOpenSSL, TestSSLTunnel) {
    using namespace std;

    // Create object nessacary for the object under test to be created
    DataManager dmServer;
    DataManager dmClient;
    IKeyTransmitter* transmitter = nullptr;
    IKeyReceiver* receiver = nullptr;
    
    // objects under test by creating a stream which uses the 
    // QTunnel as a buffer
    QTunnelServer server(&dmServer, transmitter);
    iostream serverStream(&server);

    QTunnelClient client(&dmClient, receiver);
    iostream clientStream(&client);

    // connect the tennel together
    uint16_t portnumber = 0;
    server.Listen(portnumber);

    ASSERT_TRUE(client.Connect("localhost:" + portnumber));

    // load a test string which will be sent through the tunnel
    string input = "Hello World!";
    string output;

    // send the test string through the tunnel
    serverStream << input;
    // read the output from the other side
    clientStream >> output;

    ASSERT_EQ(output, input);
    output = "";

    // repeat going in the other direction
    clientStream << input;
    serverStream >> output;

    ASSERT_EQ(output, input);

    // now test binary data.
    const char testByte[] = {42};
    char outputNumber[1] = {};

    clientStream.write(testByte, sizeof(testByte));
    serverStream.read(outputNumber, sizeof(outputNumber));
    ASSERT_EQ(outputNumber, testByte);

    serverStream.write(testByte, sizeof(testByte));
    clientStream.read(outputNumber, sizeof(outputNumber));
    ASSERT_EQ(outputNumber, testByte);
    
}
```

## CMake
### Build folders
In order to keep the source code tree free from clutter such as generated files, CMake is configured to prevent *in source builds*. CMake is run from an empty folder where the built files will be placed and given a relative path to the toplevel ```CMakeLists.txt``` file. eg:
```bash
~/CQPToolkit/build/gcc> cmake ../..
```

Then the build can be started by:
```bash
~/CQPToolkit/build/gcc> make -j
```

### Building targets
Builds can be started to run one or more [targets](https://cmake.org/cmake/help/v3.5/manual/cmake-buildsystem.7.html). The default is to run the *all* target. Some other useful targets are:

- ```doc```     Build documentation
- ```package``` Build installer package for the current platform
- ```bundle```  Build the Windows bundle installer
- ```clean```   Remove built files.

To build the documentation:

```bash
~/CQPToolkit/build/gcc> make -j doc
```

### Documentation
Documentation is generated using [Doxygen][], this has many powerful features, but at it's most basic it provides easy to read documentation with little effort from the developer.

When documenting an element of code, such as a function, variable, class etc. Simply use an extra ```/``` comment character. Further refinements can be done with [keywords](https://www.stack.nl/~dimitri/doxygen/manual/commands.html). The ```@``` prefix for keywords has been used for clarity throughout the code.

~~~cpp

/// @verbatim
/// This is a documented function.
void DoStuff();

// This is just a normal comment and will be ignored

/// This is also a documented function
/// @detail With lots of detail
/// @param value An input to the function
/// @return Something is returned
void DoLots(int value);

 /** Block comments can also be used 
 *
 */
/// @endverbatim

~~~

### CMake configuration
The ```CMakeLists.txt``` files are equivalent to the make ```Makefile```. They tell CMake how to produce the projects files which are then used to build the code. This step might seem unnecessary at first, but with large projects and multiple platforms, it has its [benefits](https://prateekvjoshi.com/2014/02/01/cmake-vs-make/).
CMake files are snippets of code which can be ```include```'d into ```CMakeLists.txt``` files.
CMake will look for a ```CMakeLists.txt``` file in the directory whenever it encounters the ```add_subdirectory()``` command.

CMake is, broadly speaking, a string parsing engine. It has conditional control:

~~~cmake
IF(TRUE)
    # ...
ENDIF()
~~~

Variables are replaced with their value when they are specified as a replacement string:

~~~cmake
SET(MY_VAR "Hello World.")
# output the variable
MESSAGE("${MY_VAR}")
~~~

Strings containing ```;``` are treated as lists.

#### An example executable program

~~~cmake
cmake_minimum_required (VERSION 3.3.2)
# Define a project to add to the build
project(SimpleAlice CXX)
# Setup some standard variables based on the build system
include(CommonSetup)
# Create a default program
CQP_EXE_PROJECT()
# In order to link the executable, add the cqp toolkit library. Replace Shared with Static for static linking
target_link_libraries(${PROJECT_NAME} CQPToolkit_Shared)
~~~

## CPack<a name="cpack"/>
CPack is part of [CMake][] and takes the files generated by the build and produces installer packages such as [DEB][], [RPM][], [MSI][], etc.
On windows Wix is used to generate the windows installer files.

To add a target (such as an executable) to the final package, use the ```INSTALL``` command

~~~cmake
INSTALL(TARGETS ${PROJECT_NAME}
    DESTINATION bin COMPONENT ${PROJECT_NAME}
)
~~~

This will put the executable generated by the ```ADD_EXECUTABLE``` command into the ```bin``` folder and create a component with the same name which can later be used to add more detail later with ```CPACK_ADD_COMPONENT```.

The package is created by building the ```package``` target. The details of how the package is configured can be found in ```CMakeModules/Packaging.cmake```

## Wix
Wix is used to generate Microsoft Windows^TM^ Installer files. The configuration is done with XML files with the extension ```.wsx```.
There are currently two installer packages created by the tool kit, one consists of all the items built by the project and is controlled by [CPack](#cpack). The other is a [Bundle][] which provides installers for all the build time dependencies.

The wix specific files can be found in ```installer```.

# FAQ
The [FAQ is here](FAQ.md)

[//]: ##Footnotes

[^1]: "A pure virtual function does not have any implementation."

[Bundle]: http://wixtoolset.org/documentation/manual/v3/bundle/
[CMake]: https://cmake.org/
[DEB]: https://en.wikipedia.org/wiki/Deb_(file_format)
[RPM]: https://en.wikipedia.org/wiki/Unix_security#RPM_packages
[MSI]: https://en.wikipedia.org/wiki/Windows_Installer
[Google C++ Style Guide]: https://google.github.io/styleguide/cppguide.html
[Doxygen]: http://www.stack.nl/~dimitri/doxygen/
