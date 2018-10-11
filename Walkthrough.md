# Toolkit walkthrough
@tableofcontents

This contributes to the uptake of QKD as a resource within a network, both in research and practical installations. It answers the  questions: "Can QKD be used in a practical environment?" and "What considerations need to be taken into account when integrating QKD with modern networks?". This work should be of interest to the security community and application developers as it solves many of the fundamental details which are currently preventing the uptake of QKD devices.

This work will show that symmetric keys can be used as a constantly supplied consumable resource. When the key can be replenished at a distance, higher security levels can be achieved. If secret key becomes compromised, it can be discarded and a fresh key used in it's place. This has knock-on effects for many of today's security concerns such as data breaches. [Drone](https://www.theregister.co.uk/2017/11/16/dji_private_keys_left_github/)

Current standard forms of encryption and authentication rely on the public key system of exchanging secrets. These are vulnerable to attack from many advances in computing power as well as the advent of quantum computing. An alternative which has been with us since the invention of cyphers is the symmetric key. With robust encryption cyphers such as AES, the attacker has only two options, brute force decryption or somehow obtain the original key. Symmetric keys generated using the principles of quantum physics are provably secure and don't need to be transferred using weaker public key systems. They're ability to be continually replenished also negates the need to perform key derivation from another, precious key obtained mandrolically, i.e. by someone physically taking a flash drive to a machine, etc.
The problem is that most of the infrastructure in use today is built around the public key system, this solution aims to be a step towards allowing use of disposable symmetric keys. Until the advent of a QKD chip in a mobile device, this system will be confined to large installations, securing physical locations such a server room, an office, etc, but the system has been designed to be flexible enough to be used as part of a standard framework such as TLS.
  
## Analysis

Device requirements.
Some devices produce raw detections, some produce ready to use key - the tool needs to handle both.
A good QKD infrastructure will require that the devices can be tasked with generating key for arbitrary point to point links, to use these resources effectively, these links will need to be changed to cope with demand, therefore the links or sessions need to be controlled during runtime not statically defined.

## Design principals

Configuration is done ether through the command line or through JSON configuration files - this is large for simplicity as GRPC has a built in JSON parser.
Clear separation of components to allow customisation of the tool with future devices. Once a key is produced it is passed to the key store where it can be managed.
Multi-language Interface definition.
GRPC was chosen as the interface middleware due to it's many supported languages. The event based system and processing requirements lend themselves to an RPC methodology rather than REST (need to justify why using REST is not always the best option \#bandwagon).

## Implementation

### Overview


    @startuml wtClassDefinitons
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

    @startuml wtSiteAgentClasses
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

### Site agent runner
Site agents control the devices and the sessions which use those devices. The site agents are run at physical locations which are equipped with QKD devices. A configuration file defines which devices are connected and how those devices are optically connected to the network. 

> diagram showing the physical connections for a QKD device with their relevant software counterparts

An example device URL : `clavis://localhost:7030/?switchname=openflow:1704&switchport=61"` defines a Clavis 2 wrapper is listening on port `7030`, the quantum channel is connected to a switch called `openflow:1704`, this will be understood by the SDN controller so that the optical port `61` is connected when key exchange is started.
The configuration file also defines the location of the network manager which the site agent will register with (see cqp::remote::INetworkManager), this is optional however and the site agents can be controlled without preregistering (see cqp::remote::ISiteAgent) as can be shown with the SiteAgentCtl tool. Site agents can register themselves with the local ZeroConf discovery agent using the `cqp::net::ServiceDiscovery` class. This allows adhoc detection of site agents.  This could be used to form a more heterogeneous network but is not compatible with many large or corporate networks.

### Statistics

During initialisation, the various classes are registered with the reporting server, this allows internal and external applications to collect data on the performance of the system (see cqp::remote::IReporting). A generic interface was designed which would allow arbitrary data to be published with minimal impact to the system performance. Statistics are reported as a hierarchy, allowing data to be grouped under categories.

    @startuml wtStats
        skinparam linetype ortho
        class Stat<T> {
            Stat(path, units)
            GetLast()
            GetAverage()
            GetTotal()
            GetMin()
            GetMax()
            Update()
            DoWork()
            Update()
            Reset()
        }
        
        interface IStatCallback<T> {
            StatUpdated(stat)
        }
        
        namespace remote {
            interface IReporting {
                GetStatistics(filter)
            }

            class ReportingFilter {
                listIsExclude
                maxRate_ms
            }     

            class StatFilter {
                fullname
            }
                       
        }
        
        ReportingFilter --> "*" remote.StatFilter
    @enduml


The producer of the statistic creates an instance of the `cqp::stat::Stat` class, defining the data type of the values and it's units (a count, time ,percentage, etc) along with any arbitrary key+value pairs.  When a value is to be updated, the producer issues the call `Update()` on the `cqp::stats::Stat` with the relevant new values.
This this then handed off to a low priority thread which calculates further values, such as minimum, maximum, averages and update times. The final collected data is then published to any listeners using the tool-kits standard `cqp::Event` class. One such listener is the `cqp::stats::ReportServer`.

    @startuml wtReporting
        skinparam linetype ortho
        
        interface IStatCallback<T> {
            StatUpdated(stat)
        }
        
        namespace remote {
            interface IReporting {
                GetStatistics()
            }
        }
        
        class ReportServer {
            ReportServer()
            GetStatistics()
            StatUpdated()
        }
        
        ReportServer -up-|> IStatCallback
        ReportServer -up-|> remote.IReporting
    @enduml

This provides the external interface `cqp::remote::IReporting` which has client side filtering, allowing each statistics consumer to define which statistics are passed on and limit the update rate. An example of the usage of the statistics capability is provided in the `StatsDump` tool.
The goal of this infrastructure was to allow the system to react to feedback from any part of the system without coupling classes unnecessarily - changes QBER can influence error correction parameters and/or passed to the network manager to influence topology decision.

### Events

The tool-kit follows an event-driven architecture to separate concerns and allow parallelisation of tasks. The flexibility has allowed the system to grow as new components have been added. To achieve this the `cqp::Event` class was designed to provide a comprehensive event system. 

    @startuml wtEvent
        skinparam linetype ortho
        interface IEvent<Interface> {
            Add(listener)
            Remove(listener)
            Clear()
        }
        
        class Interface

        class Event<Interface, \nFuncSig, \nMaxListeners> {
            Emit(args...)
        }
        
        Event -up-> IEvent
        Event -right-> "*" Interface : listeners
        
    @enduml

Interfaces can inherit `cqp::IEvent` to declare that they provide data which consumers (un)register with by inheriting the callback interface and calling `Add` or `Remove` on the consumer.  Concrete classes then inherit the `cqp::Event` class to implement the registration and publishing methods. New data is published by calling `Emit()` which notifies the listeners. The template code has been designed to allow arbitrary callback signatures.

### QKD Device Factory

The devices are managed by the `cqp::DeviceFactory` which uses the device URL to instantiate the correct driver and set-up the device. Device drivers are registered with the factory by supplying a prefix identifier which becomes the scheme portion of the URL and a static method for creating the driver. The factory maintains a list of which devices are in-use so that as devices are brought in and out of use there is no clash of usage.

    @startuml wtDeviceFactory
        skinparam linetype ortho
        class DeviceFactory {
            DeviceFactory()
            CreateDevice(url)
            UseDeviceByID(identifier)
            ReturnDevice(device)
            AddReportingCallback(callback)
            RemoveReportingCallback(callback)
            {static}GetDeviceIdentifier(url)
            {static}RegisterDriver(name, createFunc)
            {static}GetSide(url)
            {static}GetKnownDrivers()
        }
        
        interface IQKDDevice {
            GetDriverName()
            GetAddress()
            GetDescription()
            Initialise() : bool
            GetSessionController()
            GetDeviceDetails()
        }
        
        DeviceFactory --> "*" IQKDDevice : allDevices
        DeviceFactory --> "*" IQKDDevice : unusedDevices
    @enduml

### Device drivers

When the factory creates a device, it supplies the full URL of the device the channel credentials to use and the size of the keys to produce. Therefore any initialisation parameters needed by the driver can be specified in the URL. In the case of the Clavis `IDQWrapper` drivers this is the host and port number to connect to. The device drivers are responsible for creating the session controllers, this is because different devices will have different levels of control. All session controllers must implement the `cqp::ISessionController` interface and conform to the algorithm. 
This allows black-box devices such as the IDQ Clavis 2 to be managed the same way as a custom made device which can use the full tool-kit stack to produce key.

### Session controllers

Session controllers perform the set-up and tear down of devices to establish a key exchange link between two points. They are instructed by the site agents to ether start or stop a session with another session controller. The controllers start their own service port and communicate directly to establish the session, this makes them independent of the site agents. A simplified, more static configuration of devices could be created by simply starting session controllers and calling `StartSession()`. This is what is done by the `QKDSim` tool to exercise the key generation classes without including the higher level site agent and network management classes.

    @startuml wtSession
        skinparam linetype ortho
        namespace remote {
            interface ISession {
                SessionStarting(details)
                SessionEnding()
            }
        }
        
        interface ISessionController {
            StartServer(hostname, listenPort, creds)
            StartServerAndConnect(otherController, hostname, listenPort, creds)
            GetConnectionAddress()
            StartSession(parameters)
            EndSession()
            GetKeyPublisher()
            GetStats()
        }
        
        class SessionController {
            # RegisterServices(builder)
        }
        
        SessionController -up-|> ISessionController
        SessionController -up-|> remote.ISession
        
    @enduml

The key job of the specialisations of `cqp::SessionController` is the `GetKeyPublisher` call which allows the keys to be passed from an arbitrary class to the `cqp::keygen::KeyStore`. Depending on the complexity level of the driver, this may be the output of the tool-kits own `cqp::privacy::PrivacyAmplify` class in the case of fully managed devices or the result of reading bytes from a socket.

### Network manager registration

When a site agent is started with the location of a network manager, it uses the `cqp::remote::INetworkManager` interface to register it's details. These details tell the network manager which devices are available and how they can be connected to the network. When a site is shut-down, it un-register's it's self, allowing the network manager to reconfigure the network.

    @startuml wtNetworkManager
        skinparam linetype ortho
        namespace remote {
            interface ISiteAgent {
                StartNode(path)
                EndKeyExchange(path)
                GetConnectedSites()
                GetSiteDetails()
            }
            
            interface INetworkManager {
                RegisterSite(site)
                UnRegisterSite(siteAddress)
                UpdateStatistics(report)
            }
        }
        
        class SiteAgent
        
        SiteAgent -up-|> remote.ISiteAgent
        SiteAgent .right.> remote.INetworkManager
    @enduml

    @startuml wtKeyExchange

        hide footbox
        title Key Exchange Management

        actor "Quantum SDN Agent" as qsa
        control "Network Manager" as nm
        boundary "ISiteAgents" as nodeA

        note over qsa, nodeA
            This shows how the links and therefore the key rates across the network will be managed.
            Using information from the network topology and key usage, recsources will be allocated
                by instruting the site agents to generate key.
            The initial implementation of this will simply exorsise the interfaces as a proof of concept.
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
                        Details are down to future algorthms
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

### Channel credentials

### IDQWrapper

Many proprietary systems come with limitations or are configured in such a way that makes them difficult to integrate with other systems. The IDQWrapper is an example solution to such a problem. In this case the device uses hard coded ports and paths. To allow more than one device to be connected to a computer at a time, a docker image was created which runs the OEMs programs, the IDQWrapper then passes the extracted key to the session controller through the `cqp::remote::IIDQWrapper` interface. This demonstrates that almost any device can be integrated into the system.
By extending the `cqp::session::SessionController` the `cqp::session::ClavisController` can use the `StartSession` call to prepare the device with the propriety configuration parameters.

### Key stores

A `cqp::keygen::KeyStore` receives ready to use key from an `cqp::IKeyPublisher`. The key store is responsible for holding key between two points: A and B. A key store can have a path consisting of more intermediate sites, in which case it will obtain new key using the multi hop protocol. Only one key store ever exists for a given pair of sites at a given site. A key store can be disconnected from a key producing direct link between A and B, if it's path is then set to a complex chain via C, it will use the A to C  and C to B key stores to build new key for A to B.

    @startuml wtKeyStore
        skinparam linetype ortho
        title KeyStore inheritance diagram
        
        interface IKeyCallback {
            OnKeyGeneration(keyData)
        }
        
        interface IKeyStore {
            GetExistingKey(in identity, out output, in authToken)
            GetNewKey(out identity, out output, in authToken)
            DestroyKey(in identity)
            MarkKeyInUse(in identity, out alternative, in authToken)
            StoreReservedKey(in identity, in keyValue, in authToken)
            SetPath(in path)
        }
        
        class KeyStore {
            KeyStore(siteAddress, creds, destination)
        }
        
        KeyStore -up-|> IKeyCallback
        KeyStore -up-|> IKeyStore
    @enduml

    @startuml wtKeyFlow
        skinparam linetype ortho
        title Key flow through the system

        state "Transmission/Detection" as td
        state "Post Processing" as pp
        state KeyStore
        state "Multihop Keystore" as mh
        state Consumer
        
        td --> pp : IDetection
        pp --> KeyStore : IKeyCallback
        KeyStore --> Consumer : remote::IKey
        KeyStore --> mh : IKeyStore
        mh --> Consumer : remote::IKey
    @enduml


The key stores use a system of key id reservation to avoid issues with delays or race conditions. When a new key is requested, a key is allocated locally, the other site is then requested to `MarkKeyInUse` after which point, the key can only be retrieved by providing it's key id and the client authentication token. If a key is requested which doesn't yet exist, the key is marked a reserved so that when it does arrive it cannot be used by anther request.  f the key does not arrived within a time period, it is assumed to have already been used, in this case an alternative key id is reserved and sent back to the caller. This "no but try this one" approach reduces the likely hood of failure/retry during high load situations. 

### Start Node

The exchange of key is started by calling `StartNode` using the `cqp::remote::ISiteAgent` interface. The first node in the chain is given a path definition which allows it to decide whether it is at ether end or in the middle of a chain. 
A complete path consists of at least one pair of devices. Sites at intermediate hops use two devices (described as a left and a right), one connected to the previous site and one to the next site. 
The first site agent in the chain will request the device chosen for this hop from the `cqp::DeviceFactory`, thus making it unavailable for other links. The session controller is started and the connection details are passed to the next site, along with the complete path.
The site agent then connects the key publisher, obtained from the session controller, to the key store for this hop. 
If the site agent has a device on the right side of a hop, it is also started and the session controller is instructed to connect back to the first, the session controllers can now begin to exchange key while the rest of the chain is established. Any existing links which match the hop specification simply become a no-op and they continue as normal.
Once the chain is complete a key store for the entire chain is created, if it doesn't exist, by the `cqp::keygen::KeyStoreFactory`

    @startuml wtKeyExchange

        hide footbox
        title Key Exchange Algorithm

        control "SiteAgent : node1" as sa1
        boundary "ISiteAgent : nextNode" as nextNode
        participant DeviceFactory as devf
        database KeyStore

        [-> sa1 : StartNode(path, devices)
        activate sa1
        loop for each hop releated to this site

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
            sa1 -> kp1 : Add(keystore)

            note over devf, kp1
                Any key genterated by the devices key publisher
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


### Multi hop key

There are two main approaches to creating key using intermediate sites. These were categorised as the pull and push models.

#### The push model

This model establishes a key store which is fed from dedicated links, the keys generated for A to B which would normally go to key store A:B are instead fed to A:C and combined with the keys from B:C.

    @startuml wtMultiHopPush
        title Multi-hop key distrobution - Push model

        boundary "Key Publisher A" as kpa
        database "Key Store [A, B]" as ksa
        database "Key Store [A, C]" as ksb

        [-> kpa : OnKeyGeneration([A, B])
        activate kpa
            note over kpa, ksb
                A key weighting setting will allow the keys to be distibuted to the
                attached key stores. Each key will only be sent to one key store.
            end note
            alt select destination based on distrobution function
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

This requires a lot of domain knowledge to be passed down from the network manager regarding how much key is required and what is the current topology of the network. It assumes that users of the key have some pre-knowledge of the key they're going to need in the future so that it can be built up in advance. Until predictive algorithms or highly integrated clients are developed there is no data available to configure this system in a meaningful way - network traffic doesn't follow such nicely predictive flows, in reality, a client connects and wants key *now*, and they may request it again and again or not request any at all. As such a more reactive approach was implemented to match the nature of current packet based networks.

#### The pull model

This model allows key stores to sit on top of other key stores. When a session is created with a path such as A, B, C

    @startuml wtMultiHopPullKeyStores1

        skinparam linetype ortho
        
        component SiteA {
            component Device1 as sa_d1
        }

        component SiteB {
            component Device1 as sb_d1
            component Device2 as sb_d2
        }

        component SiteC {
            component Device1 as sc_d1
        }
        
        SiteA -[hidden]right- SiteB
        SiteB -[hidden]right- SiteC
        sb_d1 -[hidden]right- sb_d2
        
        sa_d1 -right- sb_d1
        sb_d2 -right- sc_d1

    @enduml

key stores are created for each hop in the chain

    @startuml wtMultiHopPullKeyStores2

        skinparam linetype ortho
        
        component SiteA {
            component Device1 as sa_d1
            database "KeyStore(A,B)" as sa_ksab
        }

        component SiteB {
            component Device1 as sb_d1
            component Device2 as sb_d2
            database "KeyStore(A,B)" as sb_ksab
            database "KeyStore(B,C)" as sb_ksbc
        }

        component SiteC {
            component Device1 as sc_d1
            database "KeyStore(B,C)" as sc_ksbc
        }
        
        SiteA -[hidden]right- SiteB
        SiteB -[hidden]right- SiteC
        sb_d1 -[hidden]right- sb_d2
        sa_d1 -[hidden]down- sa_ksab
        sb_d1 -[hidden]down- sb_ksab
        sb_d2 -[hidden]down- sb_ksbc
        sc_d1 -[hidden]down- sc_ksbc
        
        sa_d1 -right- sb_d1
        sb_d2 -right- sc_d1

    @enduml

Another key store is created in addition to these so that there is an end-to-end key store:

    @startuml wtMultiHopPullKeyStores3

        skinparam linetype ortho
        
        component SiteA {
            component Device1 as sa_d1
            database "KeyStore(A,B)" as sa_ksab
            database "KeyStore(A,C)" as sa_ksac
        }

        component SiteB {
            component Device1 as sb_d1
            component Device2 as sb_d2
            database "KeyStore(A,B)" as sb_ksab
            database "KeyStore(B,C)" as sb_ksbc
        }

        component SiteC {
            component Device1 as sc_d1
            database "KeyStore(B,C)" as sc_ksbc
            database "KeyStore(A,C)" as sc_ksac
        }
        
        SiteA -[hidden]right- SiteB
        SiteB -[hidden]right- SiteC
        sb_d1 -[hidden]right- sb_d2
        sa_d1 -[hidden]down- sa_ksab
        sb_d1 -[hidden]down- sb_ksab
        sb_d2 -[hidden]down- sb_ksbc
        sc_d1 -[hidden]down- sc_ksbc
        sa_ksab -[hidden]down- sa_ksac
        sc_ksbc -[hidden]down- sc_ksac
        
        sa_d1 -right- sb_d1
        sb_d2 -right- sc_d1

    @enduml

The new key store is linked to the existing key stores and the sites along the chain.

When a key is requested between A and C, the key store (assuming all the stored key has been exhausted) contacts the last site in the chain and request that it builds a new key using the `cqp::remote::ISiteAgent::BuildXorKey()` method. The key store factory then walks back through the chain requesting that the previous site sends it the XOR of two keys. The final key is produced by both ends of the chain XOR'ing their respective keys. 
By building links in this way, keys can be derived from multiple links while still maintaining the secrecy of the final key.
The key can then be issued as normal.


    @startuml wtMultiHopPull
        title Multi-hop key distrobution - Pull model
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

            par #transparent In parrallel with BuildXorKey
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
                                There is no id for this side so get a new key
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

### QKD post processing interfaces

### Interface considerations

Any high volume data paths use gRPC streams.

### CMake

## TLS and preshared keys

This can be achieved by implementing the OpenSSL PSK callbacks (`SSL_CTX_set_psk_server_callback` and `SSL_CTX_set_psk_client_callback`)
See [here](https://bitbucket.org/tiebingzhang/tls-psk-server-client-example/src/783092f802383421cfa1088b0e7b804b39d3cf7c/psk_server.c?at=default&fileviewer=file-view-default) for example PSK server

> Note: TLS1.3 has different callbacks and looses the identity hint - which is why it's not used in the following

    @startuml

        box "Lan A" #LightBlue
        actor Client as cl
        database KeyStoreA as ksa
        end box

        box "Lan B" #LightGreen
        database KeyStoreB as ksb
        control Server as srv
        end box

        activate cl
        cl -> srv : GET("/KeyStoreAddress")
        activate srv
        note over ksb,srv
            This could added as an extension
            to the ServerHello message
        end note
        cl <-- srv : KeyStoreB:1234
        deactivate srv
        note over cl, ksa
            local keystore detected with ZeroConf
        end note

        cl -> ksa : GetKey("KeyStoreB:1234")
        activate ksa
        cl <-- ksa : KeyID, KeyValue
        deactivate ksa

        == TLS handshake begins ==

        cl -> srv : ClientHello(TLS_DHE_PSK)
        activate srv
        cl <-- srv : ServerHello
        deactivate srv
        cl -> srv : ClientKeyExchange(PSK_Identity=keysrv://keyStoreA:5678/KeyID)
        activate srv
        srv -> ksb : GetKey("keyStoreA:5678", KeyID)
        activate ksb
        srv <-- ksb : KeyValue
        deactivate ksb

        cl <-- srv : ChangeCipherSpec
        deactivate srv

        == TLS Protocol continues ==
    @enduml

[PKCS#11 URIs](https://tools.ietf.org/html/rfc7512)
Doesn't seem to be any standard way of getting the psk for a service. PSCK11 doiesn't really cut it.

### Android

Android has PSK ciphers as ov API 21. Enabled by supplying a PSKManager to the SSLContext when creating the engine. See [SSLEngine](https://developer.android.com/reference/javax/net/ssl/SSLEngine)

Android has a [psk manager](https://developer.xamarin.com/api/type/Android.Net.PskKeyManager/). identity hints are limited to 128bits!
Java library bouncycastle (may need to use spongycastle as android version is crippled) has TLS PSK support

## Demonstration

Demonstrated on Ubuntu 18.04
