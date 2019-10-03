Cryptographic tunnels
=====================
@tableofcontents

## Creating the tunnels

Once the controllers have been configured, they wait for `cqp::net::ServiceDiscovery` to detect the other controller, the event is handled by the `cqp::tunnels::Controller::OnServiceDetected` method which ultimately calls the `cqp::tunnels::TunnelBuilder` to establish the tunnel. `cqp::tunnels::Controller` is also responsible for registering it's controller for broadcast so that other controllers can find it.

```plantuml
    @startuml CommunicateSecurely

        title Communicate securely

        rectangle QTunnelServer {
            usecase (Exchange raw data) as erd
        }
        actor ClientA
        actor ClientB

        usecase (Aquire a common key) as ck
        erd -[hidden]d-> ck

        ClientA -r-> erd
        ClientB -l-> erd
        ClientA -r-> ck
        ClientB --> ck

        erd .d.> ck : include

    @enduml
```

## Site Agent

The QKD devices, connections and keys are managed by a SiteAgent.

```plantuml
    @startuml SystemOverview
        title System overview

        node "Site" {
            [SiteAgent]
            () ISiteAgent
            ISiteAgent -down- [SiteAgent]
        }
        note top of Site
            A physical location around
            which is our trusted node
        end note

        node SDN {

            [NetworkManager]
            () INetworkManagement
            note top of [NetworkManager]
                Provides awareness
                of network state
            end note
            INetworkManagement -left- [NetworkManager]

            [QuantumPathAgent]
            () IQuantumPath
            note right of [QuantumPathAgent]
                Provides path discovery
                between sites
            end note
            IQuantumPath -down- [QuantumPathAgent]

            [NetworkManager] -[hidden]down- IQuantumPath
        }

        [NetworkManager] -down-( IQuantumPath
        [SiteAgent] -right-( INetworkManagement


    @enduml
```

> Interfaces:
> cqp::remote::INetworkManager,
> cqp::remote::IQuantumPath,
> cqp::remote::ISiteAgent
___

```plantuml
    @startuml KeyClassDefinitions
        title Class definitions

        class SiteAgent
        note right of SiteAgent
            Controls the local site
        end note

        class SessionController
        note right of SessionController
            Builds key between it's
            self and another site
        end note

        class KeyStoreFactory
        note right of KeyStoreFactory
            Holds the available
            instances of KeyStore
            Create new instances on request
        end note

        class KeyStore
        note right of KeyStore
            Stores usable key between
            itself and another Site
        end note

        class QKDDevice
        note right of QKDDevice
            A physical device which produces
            key with a paired device.
        end note

        class DeviceFactory
        note right of DeviceFactory
            Provides access to the
            available devices in the Site
        end note

        class KeyPublisher
        note right of KeyPublisher
            Emits keys as they become
            available from the device
        end note

        SiteAgent -[hidden]down- SessionController
        SessionController -[hidden]down- KeyStore

        KeyStoreFactory -[hidden]down- DeviceFactory
        DeviceFactory -[hidden]down- QKDDevice

        QKDDevice -[hidden]down- KeyPublisher
    @enduml
```

> Classes:
> cqp::DeviceFactory,
> cqp::KeyPublisher,
> cqp::keygen::KeyStore,
> cqp::keygen::KeyStoreFactory,
> cqp::SiteAgent,
> cqp::SessionController

___

```plantuml
    @startuml SiteAgentClasses
        title SiteAgent classes

        together {
            interface remote::ISiteAgent {
                StartNode()
                EndKeyExchange()
                GetConnectedSites()
            }

            class SiteAgent {
                configuration
            }

            SiteAgent -up-|> remote::ISiteAgent
            SiteAgent -left-> "*" remote::ISiteAgent : otherSites


            interface remote::ISession {
                SessionStarting()
                SessionEnding()
            }
        }

        together {
            class SessionController {
                StartServer()
                StartServerAndConnect()
                StartSession()
                EndSession()
                GetKeyPublisher()
            }

            class KeyStoreFactory {
                GetKeyStore(path)
            }

            class DeviceFactory {
                GetDevice(address)
            }

        }

        class KeyStore {
            GetKey()
        }

        together {
            class QKDDevice {
                GetSessionController()
            }

            class KeyPublisher
        }

        SiteAgent ..> SessionController : <<uses>>
        SiteAgent *--> "1" DeviceFactory
        SiteAgent *-left-> "1" KeyStoreFactory

        SessionController -up-|> remote::ISession
        SessionController --> remote::ISession : partner

        SessionController *-down-> KeyPublisher

        KeyStoreFactory *--> "*" KeyStore

        DeviceFactory *--> "*" QKDDevice
        QKDDevice *-up-> SessionController

    @enduml
```

> Classes:
> cqp::DeviceFactory,
> cqp::keygen::KeyStore,
> cqp::keygen::KeyStoreFactory,
> cqp::SessionController,
> cqp::SiteAgent
>
> Interfaces:
> cqp::IQKDDevice,
> cqp::IKeyStore,
> cqp::IKeyPublisher,
> cqp::remote::IKeyFactory,
> cqp::remote::ISession,
> cqp::ISessionController

## Key Manager Algorithm

```plantuml
    @startuml NodeConnections

        title Node connections

        [SiteA]
        [SiteB]
        [SiteC]
        [SiteD]
        [SiteE]
        [SiteF]

        SiteA -- SiteB : <&key>1
        SiteB -- SiteF : <&key>6
        SiteB -- SiteC : <&key>2
        SiteB -- SiteE : <&key>4
        SiteC -- SiteD : <&key>3
        SiteE -- SiteD : <&key>5

        SiteE -[hidden]r- SiteC
    @enduml
```

In order for the data transfer to begin, the symmetric key must first be exchanged, when more that two nodes are involved It was decided that key xor'ing would be used.  This allows multiple nodes in the chain without too much overhead.

```plantuml
    @startuml KeyExchangeManagement

        hide footbox
        title Key Exchange Management

        actor "Quantum SDN Agent" as qsa
        control "Network Manager" as nm
        boundary "ISiteAgents" as nodeA

        note over qsa, nodeA
            This shows how the links and therefore the key rates across the network will be managed.
            Using information from the network topology and key usage, resources will be allocated
                by instructing the site agents to generate key.
            The initial implementation of this will simply exorcise the interfaces as a proof of concept.
        end note

        activate nm
        loop Maintain key generation.
            note over nm, nodeA
                Sites register with the manager as they start. The device definitions will
                contain the polatis port names needed for the QPA
            end note
            nm /- nodeA : Register(address, devices)

            note over nm, nodeA
                Sites continue to report their statistics such as keystore levels/rates
            end note
            nm /- nodeA : UpdateStatistics(address, stats)

            alt A topology change is required
                note over nm, nodeA
                    This may occour for a number of reasons, for default setup
                    this could be as simple as a table of connections to start
                    exchanging key
                end note

                loop as required
                    nm -> qsa : GetPath()
                    note over qsa, nm
                        Some algorithms may decide to recall this to find better connections.
                        eg, A-B already exists, A-C is required. This could ether reuse A-B + B-C
                            or A-C could be created to generate a higher key rate.
                        Details are down to future algorithms
                    end note
                end loop

                alt if a new connection is to be created
                    nm -> qsa : CreatePath(newPath)
                end alt

                alt If an existing key exchange needs to be stopped
                    nm -> nodeA : EndKeyExchange(oldPath)
                    nm -> nm : UpdateTopologyData()
                end alt

                alt If a new key exchange needs to be started
                    note over nm, nodeA
                        Instruct the SiteAgent to begin creating key for the path.
                    end note
                    nm -> nodeA : StartNode(path, devices)
                    nm -> nm : UpdateTopologyData()
                end alt
            end alt
        end loop
        deactivate nm

    @enduml
```

> Interfaces:
> cqp::remote::INetworkManager,
> cqp::remote::IQuantumPath,
> cqp::remote::ISiteAgent

___

```plantuml
    @startuml KeyExchangeAlgorithm

        hide footbox
        title Key Exchange Algorithm

        control "SiteAgent : node1" as sa1
        boundary "ISiteAgent : nextNode" as nextNode
        participant KeyStoreFactory as ksf
        participant DeviceFactory as devf
        database KeyStore

        [-> sa1 : StartNode(path, devices)
        activate sa1
        loop for each hop related to this site

            sa1 -> devf : UseDeviceById(criteria)
            activate devf
                create participant "QKDDevice" as dev1
                devf -> dev1 : new
                activate dev1
                    create participant "Session Controller" as sc1
                    dev1 -> sc1 : new
                    activate sc1
                        create participant "IKeyPublisher" as kp1
                        sc1 -> kp1 : new
                    deactivate sc1
                deactivate dev1
            deactivate devf

            sa1 -> sc1 : StartServer()
            sa1 -> ksf : GetKeyStore(criteria)
            sa1 -> kp1 : Add(keystore)

            note over devf, kp1
                Any key generated by the devices key publisher
                will be sent to the key store
            end note

            alt if this site is the first node in this hop

                sa1 -\ nextNode : StartNode(path)
                activate nextNode
                deactivate nextNode
            else if this site the second node in the hop
                sa1 -> sc1 : StartServerAndConnect(previousNodeAddress)
                sa1 -> sc1 : StartSession()
            end alt
        
            alt If there is more than one hop
                sa1 -> KeyStore : SetPath(path)
            end alt
        end loop

        deactivate sa1

    @enduml
```

> Classes:
> cqp::DeviceFactory,
> cqp::keygen::KeyStore,
> cqp::keygen::KeyStoreFactory,
> cqp::SessionController,
> cqp::SiteAgent
>
> Interfaces:
> cqp::IQKDDevice,
> cqp::IKeyStore,
> cqp::IKeyPublisher,
> cqp::remote::IKeyFactory,
> cqp::remote::ISession,
> cqp::ISessionController

### Multi-hop

There are two possible ways of distributing the keys needed for multi hop. One is to pre-allocate a percentage of local keys to the
multi-hop key store:

```plantuml
    @startuml Multi-hopKeyDistrobution-PushModel
        title Multi-hop key distribution - Push model

        boundary "Key Publisher A" as kpa
        database "Key Store [A, B]" as ksa
        database "Key Store [A, C]" as ksb

        [-> kpa : OnKeyGeneration([A, B])
        activate kpa
            note over kpa, ksb
                A key weighting setting will allow the keys to be distributed to the
                attached key stores. Each key will only be sent to one key store.
            end note
            alt select destination based on distribution function
                kpa -> ksa
            else
                kpa -> ksb
            end alt
        deactivate kpa

        ...

        [-> ksa : GetSharedKey()
        ...
        [-> ksb : GetSharedKey()

    @enduml
```
The other is to pull keys as needed from the stores:

```plantuml
    @startuml Multi-hopKeyDistrobution-PullModel
        title Multi-hop key distribution - Pull model
        hide footbox

        box "Site A"
        database "Keystore A,C" as ksac_a
        database "Keystore A,B" as ksab_a
        end box

        box "Site B"
        database "Keystore A,B" as ksab_b
        boundary "Site Agent B" as sab
        database "Keystore B,C" as ksbc_b
        end box

        box "Site C"
        database "Keystore B,C" as ksbc_c
        boundary "Site Agent C" as sac
        database "Keystore A,C" as ksac_c
        end box

        [-> ksac_a : GetNewKey()
        activate ksac_a

            ksac_a -> ksab_a : GetNewKey()
            activate ksab_a

        note right #aqua
            Greek letters represent key IDs, on their own they do not contain any key values.
            The key function: key(α) represents a complete key, both value and id
        end note

                ksac_a <-- ksab_a : key(α)
            deactivate ksab_a

            ksac_a -\ sac : BuildXorKey(α, [A,B,C])
            activate sac

            par #transparent In parallel with BuildXorKey
                [<-- ksac_a : key(α)
            also
                sac -> ksbc_c : GetNewKey()
                activate ksbc_c
                sac <-- ksbc_c : key(β)
                deactivate ksbc_c

                sac -> sab : GetCombinedKey([A,B]=α, [B,C]=β)
                activate sab
                    note left
                        Request a key from A to B and B to C use key ids α and β
                    end note
                    alt left id supplied
                        sab -> ksab_b : GetExistingKey(α)
                        activate ksab_b
                            sab <-- ksab_b : key(α)
                        deactivate ksab_b
                    else
                        sab -> ksab_b : GetNewKey()
                        activate ksab_b
                            note left
                                There is no id for this
                                side so get a new key
                            end note
                            sab <-- ksab_b : key(γ)
                        deactivate ksab_b
                    end alt
                    sab -> ksbc_b : GetExistingKey(β)
                    activate ksbc_b
                        sab <-- ksbc_b : key(β)
                    deactivate ksbc_b
                    
                    hnote over sab #coral
                        key(δ) ≔ key(α)⊕key(β)
                    end note
                    sac <-- sab : key(δ),[A,B]=α
                    note left
                        Pass back the generated key and the id for the A to B key
                    end note

                deactivate sab

                hnote over sac #coral
                    key(α) ≔ key(β)⊕key(δ)
                end note
                sac -> ksac_c : StoreReservedKey(key(α))
                note left
                    A user can now call GetExistingKey to use the key
                end note
            deactivate sac

            end par
        deactivate ksac_a

    @enduml
```

## Interfaces

Interfaces between instances, such as those between the site and the SDN will be defined with [gRPC](https://grpc.io/). The interfaces are defined using the [Protocol Buffer IDL](https://developers.google.com/protocol-buffers/) with gRPC extensions. They are built into a separate QKDInterfaces library.

## Future work

* Keys should be managed and destroyed, the requester could also specify the use of the key to allow the decision to be made.
* The QPA parameters which the QPA uses to decide on a path need to be fleshed out, data such as current load could be reported by the nodes.
* Interfacing to IDQuantique Devices - Done
* Investigate complementary technologies:
    + [MACsec](https://developers.redhat.com/blog/2016/10/14/macsec-a-different-solution-to-encrypt-network-traffic/) and [The MACsec Key Agreement - MKA](https://www.ietf.org/proceedings/76/slides/msec-5.pdf)
    + Key Management Interoperability Protocol (KMIP)
        - [Implementations](https://wiki.oasis-open.org/kmip/KnownKMIPImplementations)
        - [pyKMIP](https://pykmip.readthedocs.io/en/latest/installation.html)

## Clavis 2

There are some issues with interfacing with the USB Clavis 2 device:

- The test programs provided (QKDSequence/QKDMenu) use hardcoded port numbers and device identifiers which mean that only one instance can be run at a time
- The developmental libraries provided are not fit for purpose.
    - They are 32 bit only
    - Built for an OS which is no longer supported (Ubuntu 12.04)
    - Still have hard coded port numbers and identifiers
    - Have external dependencies to unknown versions of other libraries (boost)

### The IQSequence program

```plantuml
    @startuml Interfaces-Clavis2ConectionDiagram

        title Interfaces - Clavis 2 Connection Diagram

        skinparam node {
                borderColor Green
                backgroundColor Yellow
        }

        component [IDQSequence A] as seqA

        node "Clavis2 A" as clavisA

        component [IDQSequence B] as seqB

        interface "USB 1DDC:020[3,4]" as usb
        interface "UDP 50001-50002?" as ipc
        interface "UDP 5323" as key

        seqA -r- ipc
        seqB -l- ipc

        seqA -u- key
        seqA -- usb
        clavisA -u- usb

        note right of key
            Key retrieved by sending UDP packet to this port
            The key is then sent @t+n by another UDP packet
        end note

        note bottom of ipc
            Communication between pair
            for key distillation
        end note

        note right of clavisA
            IDQSequence attaches to the first
            device with a matching id (for alice/bob)
        end note
    @enduml
```
### Possible solution

By wrapping the program in a container, the network interfaces can be controlled

- Can a VLAN be used to isolate all the IDQSequence communications?

```plantuml
    @startuml Interfaces-Clavis2ConectionDiagram

        title Interfaces - Clavis 2 Connection Diagram

        skinparam node {
                borderColor Green
                backgroundColor Yellow
        }

        package "Docker container A" {
            component [IDQSequence A] as seqA

            interface "USB 1DDC:020[3,4]" as usb
            interface "UDP 5323 -> host:someport" as key
        }

        interface "host port" as hostkey
        node "Clavis2 A" as clavisA

        package "VLAN?" {
            interface "UDP 50001-50002?" as ipc
        }

        package "Docker container B" {
            component [IDQSequence B] as seqB
        }

        seqA -r- ipc
        seqB -l- ipc

        seqA -u- key
        key -u- hostkey
        seqA -- usb
        clavisA -u- usb

        note right of key
            Host interface docker mapping
            used to change exposed port:
            "-p 127.0.0.1:6001:5323/udp"
        end note

        note bottom of ipc
            attached to a virtual/swarm network
        end note

        note right of clavisA
            Host device mapped by providing access to /proc
            and particular usb device:
            "~--privileged ~--device /dev/usb/x/y/z -v /dev/bus/usb:/dev/bus/usb"
        end note
    @enduml
```

This solution has a number of issues:

- The network overlay which allows the containers to talk directly to each other requires a docker swarm with a keystore. This is faily easy to setup but is not trivial

### Docker setup

Be carful that any bridge you create doesn't interfere with existing networks/subnets
This assumes that the hosts involved have all joined the same swarm.

Create an overlay network specifically for the programs to talk to each other (ipc = inter process communication) called idq-ipc. ``attachable`` allows it to be used by normal containers - otherwise we'd need to create a service which is more complicated.

** We can ether create the network with internal - in which case port mapping with -p will be needed or without and use the bridge network with an ip address **

- internal
    * different port numbers all on localhost
    * udp difficult
- external
    * need to find the ip on the bridge interface for the container
    * udp works as is
    * same port numbers on different addresses

```bash
    sudo docker network create -d overlay --attachable idq-ipc --internal --subnet 192.168.103.0/24
```
The Docker file for configuring the system

```Dockerfile

```

Run the wrapper program which will wait for a command to start the IDQSequence program.
The name given to the contain can be used by other contains to contact it.

```bash
    sudo docker run --rm --privileged --network idq-ipc --name <somename> -p 127.0.0.1:5323:5323/udp --device /dev/usb/x/y/z -v /dev/bus/usb:/dev/bus/usb idqcontainer
```

## Current implementation

The QTunnelServer project (located under the tools folder) creates an encrypted tunnel using a settings file. It takes two instances of the QTunnelServer program, one for each end of the tunnel. The tunnel is defined by an cqp::tunnels::TunnelID for the tunnel and a cqp::tunnels::NodeIndex for either end of the tunnel.

QTunnelServer uses building blocks to implement one solution to the problem of sending encrypted data from one site to another. This solution assumes that the users do not/cannot have knowledge of QKD. This applies to most pre-existing systems which do not expose their communication mechanism.

Both the client entry/exit points and the channel for encrypted data can be any of:

* Physical ethernet port
* tcp port (as ether a server or client)
* udp port

```plantuml
    @startuml QTunnelServerExampleUse

        title Example use
        skinparam BoxPadding 200

        component [ClientA : Laptop] as ca
        component [SiteA : QTunnelServer] as sa
        component [SiteB : QTunnelServer] as sb
        component [ClientB : Laptop] as cb

        interface Ethernet as etha
        interface Ethernet as ethb
        interface "TCP Server" as tcp

        ca .r. etha
        etha -r- sa
        sa .d. tcp
        tcp -d- sb
        sb -r- ethb
        ethb .r. cb

    @enduml

    @startuml QTunnelServerParticipants

        set namespaceSeparator ::
        title QTunnelServer participants

        class QTunnelServer
        class Controller
        class TunnelBuilder {
            + ConfigureEndpoint()
            # EncodingWorker()
        }
        class DeviceIO {
            WaitUntilReady()
            Read()
            Write()
        }

        interface ITunnelServer {
            StartTunnel()
            CompleteTunnel()
        }
        interface ITransfer {
            Transfer()
        }

        Controller -u-|> ITunnelServer
        QTunnelServer --> "1" Controller
        Controller -u-> ITunnelServer : peer
        Controller --> "*" TunnelBuilder
        TunnelBuilder -u-|> ITransfer
        TunnelBuilder --> ITransfer : peer
        TunnelBuilder --> DeviceIO : client
    @enduml
```

Most of these classes build the environment for cqp::tunnels::TunnelBuilder to work effectively. The interfaces isolate the two halves of the tunnel. The cqp::net::Remoting class allows the interfaces to be translated into JSON strings, currently these are used over a standard tcp connection but this could be translated to a more formal interface without affecting the worker classes.

```plantuml
    @startuml TunnelBuilderAssociations

        set namespaceSeparator ::
        title TunnelBuilder associations

        class TunnelBuilder
        interface ITunnel
        interface IKeyStore

        class DeviceIO
        class Encryptor
        class Decryptor
        class KeyStore
        class RandomNumberGenerator
        class PKIChannel

        KeyStore -u-|> IKeyStore

        TunnelBuilder -u-> "1" ITunnel : farSideEP
        TunnelBuilder *-r- "1" DeviceIO : dataChannel
        TunnelBuilder *-- "1" DeviceIO : client
        TunnelBuilder *-- "1" Encryptor
        TunnelBuilder *-- "1" Decryptor
        TunnelBuilder *-l- "1" KeyStore
        TunnelBuilder *-- "1" RandomNumberGenerator
        TunnelBuilder *-- "1" PKIChannel : pkiChannel

    @enduml
```

> Classes:
> cqp::tunnels::TunnelBuilder,
> cqp::keygen::KeyStore,
> cqp::DeviceIO,
> cqp::tunnels::GCM

The cqp::tunnels::TunnelBuilder generates a private and public key which allows both sides to establish an initial shared secret, this can be used for several things:

* If the key exchange mode is set to public then this key is used for all encryption/decryption
* If the key exchange mode uses the clavis 2 devices, this shared secret is used in the IDQ protocol as the initial key

cqp::ITunnel::GetPublicKey is used to get the key from the other side.

The settings can be configured by the QKDTunnel program.

### Documentation for cqp::tunnels::TunnelBuilder::ConfigureEndpoint

@copydoc cqp::tunnels::TunnelBuilder::ConfigureEndpoint

The threads then go to work processing information as it come in

### Encoding

@copydoc cqp::tunnels::TunnelBuilder::EncodingWorker

### Decoding

@copydoc cqp::tunnels::TunnelBuilder::Transfer

## QTunnelServer

The agent program loads it's settings from a json file to create tunnels. See cqp::

### cqp::IServiceCallback

Services are discovered and broadcast by cqp::net::ServiceDiscovery when the services
available changes cqp::IServiceCallback::OnServiceDetected is called.

### cqp::ITunnel

The settings for the controller can be modified with cqp::remote::tunnels::ITunnelServer::ModifyTunnel

#### cqp::ITunnel::ModifyTunnel

Passing in cqp::tunnels::Tunnel
@copydoc cqp::remote::tunnels::ITunnelServer::ModifyTunnel

#### Finally the tunnel is started with cqp::ITunnel::CreateEndpoint

Passing in cqp::tunnels::TunnelID

@copydoc cqp::remote::tunnels::ITunnelServer::StartTunnel
