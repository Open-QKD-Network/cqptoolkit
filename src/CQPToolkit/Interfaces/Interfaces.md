# Toolkit Interfaces

## QKD

    @startuml
        title Class definitions

        interface ISiteAgent {
            StartNode(PhysicalPath)
            EndKeyExchange(PhysicalPath)
            GetConnectedSites()
            GetSiteDetails()
        }
        note right of ISiteAgent
            Controls a QKD site
        end note

        interface IKey {
            GetSharedKey(KeyRequest)
        }
        note right of IKey
            Provides access to shared keys
        end note

        interface INetworkManager {
            RegisterSite(Site)
            UnregisterSite(SiteAddress)
            UpdateStatistics(SiteAgentReport)
        }
        note right of INetworkManager
            Controls the Site agent sessions
        end note


        ISiteAgent -[hidden]down- IKey
        IKey -[hidden]down- INetworkManager
    @enduml


## Tunneling

    @startuml
        title Class definitions

        interface ITunnelServer {
            GetSupportedSchemes()
            GetControllerSettings()
            ModifyTunnel(Tunnel)
            DeleteTunnel(Name)
            StartTunnel(Name)
            StopTunnel(Name)
            CompleteTunnel(CompleteTunnelRequest)
        }
        note right of ITunnelServer
            Controls the setup of encrypted tunnels
        end note

        interface IEncryptedData {
            OnEncryptedData(EncryptedDataValues)
        }
        note right of IEncryptedData
            Transfers encrypted data
        end note

        ITunnelServer -[hidden]down- IEncryptedData

    @enduml
