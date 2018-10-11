@page KNP Key Negotiation Protocol
@tableofcontents

@details
@startuml(id=KeyExchange)
title "Key Exchange"

participant "A Listeners:IKey" as AKey
participant "B Listeners:IKey" as BKey
== Initialise ==
IKeyReceiver -> IKeyTransmitter:RequestCapabilities
IKeyTransmitter -> IKeyReceiver:ReturnCapabilities
== Continuous Key Generattion ==

IKeyReceiver -> IKeyTransmitter:RequestKeySession
IKeyTransmitter -> IKeyReceiver:InitSession

loop frames per key
    IKeyReceiver -> IKeyTransmitter:ReadyToReceive
    note over IKeyTransmitter
        Transfer quantom states
    end note
    IKeyTransmitter -> IKeyReceiver:FrameComplete
end loop
== Post Processing ==
note  over IKeyTransmitter, IKeyReceiver
See [[d0/d6b/_p_p.html Post Processing page]]
end note

== Key generated ==
IKeyTransmitter -> AKey:Emit
IKeyReceiver -> BKey:Emit

@enduml
