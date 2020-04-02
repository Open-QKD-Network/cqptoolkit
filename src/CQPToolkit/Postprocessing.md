# Post Processing
While there are many methods of QKD randomness sharing, the raw data produced by the physical devices must be processed to some degree or another to produce a usable - verifiable secure key. This process has yet to be standardised, the tool-kit offers classes and interfaces to take some steps towards standardisation.
The tool-kit has split the post-processing into stages, defined by internal and external interfaces, for the purpose of post processing, the external interfaces provide a means for each stage to exchange information. Once each stage has processed enough information, it passes it to the next using the standard event system. Each stage is *not* hard wired to the next, allowing different implementations to be selected at runtime. In this way, different algorithms for, say, error correction can be tested and compared without affecting the rest of the system. With more intelligence, different algorithms could be selected based on feedback from the system it's self.

- *Detection* - The entry point for qubits into the processing pipeline
    + `cqp::IPhotonEventCallback` is implemented by the first processing stage and is fed with ether data from the detector or results of actual transmission
- *Alignment* - Synchronisation of data point so that both sides agree on what is the first detection
    + The stage communicates using the `cqp::remote::IAlignment` interface
- *Sifting* - The process of removing qubits which weren't detected.
    + The stage communicates using the `cqp::remote::ISift` interface
- *Error Correction* - Removing or correcting errors in the detections to produce bytes which are known to be identical on both sides.
    + The stage communicates using the `cqp::remote::IErrorCorrect` interface
- *Privacy Amplification* - Reduce or eliminate any information Eve may have about the key
    + The stage communicates using the `cqp::remote::IPrivacyAmplify` interface
- *Key generation* - Publishing the final usable key
    + The last stage will pass the final key to an implementer of `cqp::IKeyCallback`

The complete chain is controlled by a single session controller.

## Data Flow

The two sides need to pass information between each other, this information is specific to the process being performed.

    @startuml PostProcessingPipelineClasses
        title Post Processing Pipeline Classes
        set namespaceSeparator ::
        skinparam linetype ortho
        
        namespace remote {
        Interface "IAlignment" as IAlign
        Interface "ISift" as ISift
        Interface "IErrorCorrect" as IError
        Interface "IPrivacyAmplify" as IPriv
        }
        
        Interface IPhotonEventCallback
        class Alignment
        
        Alignment -up-|> IPhotonEventCallback
        Alignment -d-> remote::IAlign
        
        Interface IAlignmentCallback
        class Sifting
        
        Sifting -up-|> IAlignmentCallback
        Sifting -d-> remote::ISift
        Alignment -[hidden]r-> Sifting
        
        Interface ISiftedCallback
        class ErrorCorrection
        
        ErrorCorrection -up-|> ISiftedCallback
        ErrorCorrection -d-> remote::IError
        Sifting -[hidden]r-> ErrorCorrection
        
        Interface IErrorCorrectCallback
        class PrivacyAmplificaiton
        
        PrivacyAmplificaiton -up-|> IErrorCorrectCallback
        PrivacyAmplificaiton -d-> remote::IPriv
        ErrorCorrection -[hidden]r-> PrivacyAmplificaiton
    @enduml

    @startuml PostProcessingPipeline
        title Post Processing Pipeline
        hide footbox

        control Alignment as Align
        control Sift
        control ErrorCorrection as EC
        control PrivacyAmp as PA
        database KeyStore as ks
            
        [->> Align:OnPhotonReport(Qubits)

        activate Align
        Align -> Align : Emit()
        activate Align
        Align ->> Sift:OnAligned(Qubits)

        deactivate Align
        deactivate Align
        
        activate Sift
        Sift -> Sift : Emit()
        activate Sift
        
        Sift ->> EC:OnSifted(bits)

        deactivate Sift
        deactivate Sift
        
        activate EC
        EC -> EC : Emit()
        activate EC
            EC ->> PA:OnCorrected(bits)
        deactivate EC
        deactivate EC
        
        activate PA
        PA -> PA : Emit()
        activate PA
        PA ->> ks:OnKeyGeneration(n*keys)
        deactivate PA
        deactivate PA
        activate ks
        deactivate ks

    @enduml

## Communication between Alice and Bob

Each stage uses an interface to abstract the calls needed for communication, this allows the method of communication to be seperated from the implementation.

### Alignment inter-communication
Alignment data is passed to the other side using the cqp::remote::IAlignement interface.
Results are published using the IAlignCB interface

    @startuml AlignmentProcessing
        hide footbox
        title Alignment Processing
        participant "Alignment : Bob" as Bob
        boundary "Alignment : Alice" as AIA 
        boundary "IAlignCallback : Bob" as BIACB 
        boundary "IAlignCallback : Alice" as AIACB 

        loop
        activate Bob
        Bob -> Bob:WaitForData

        note right
        OnPhotonReport triggers processing
        end note

        Bob -> AIA:AlignmentData
        activate AIA
        AIA ->> AIACB:OnAligned
        deactivate AIA
        note left
        Valid data is passed to the next stage
        end note

        Bob ->> BIACB:OnAligned
        end loop
        deactivate Bob
    @enduml


### Sifting inter-communication
Sifting data is passed to the other side using the ISift interface.
Results are published using the ISiftCallback interface

    @startuml Sifting
        hide footbox
        title Sifting
        participant "Bob"
        boundary "Alice:ISift" as AIS [[d1/d1a/classcqp_1_1_i_sift.html]]
        boundary "Bob:ISiftCallback" as BISCB [[d5/d1b/classcqp_1_1_i_sift_callback.html]]
        boundary "Alice:ISiftCallback" as AISCB [[d5/d1b/classcqp_1_1_i_sift_callback.html]]

        activate Bob
        loop
        Bob -> Bob:WaitForData
        Bob -> AIS:VerifyBases
        activate AIS
        AIS ->> AISCB:OnSifted
        note left
        Valid data is passed to the next stage
        end note
        Bob <-- AIS:Answers
        deactivate AIS
        Bob ->> BISCB:OnSifted
        end loop

        deactivate Bob
    @enduml


### Error correction inter-communication
Error correction data is passed to the other side using the IErrorCorrect interface.
Results are published using the IErrorCorrectCB interface.

    @startuml ErrorCorrection
        hide footbox
        title Error Correction
        control "Bob"
        control "Alice"
        boundary "Alice:IErrorCorrect" as AIEC
        boundary "Bob:IErrorCorrectCallback" as BIECCB
        boundary "Alice:IErrorCorrectCallback" as AIECCB

        activate Bob
        activate Alice
        loop
        Bob -> Bob:WaitForData
        note right
        The communication could also go from alice to bob
        end note
        Bob -> Bob:GenerateParity
        activate Bob
        deactivate Bob
        Alice ->Alice:WaitForData
        Alice -> Alice:GenerateParity
        activate Alice
        deactivate Alice

        Bob -> AIEC:NotifyDataParity
        activate AIEC
        AIEC -> AIEC:CompareParity
        AIEC ->> AIECCB:OnCorrected
        note left
        Valid data is passed to the next stage
        end note
        Bob <-- AIEC:Result
        deactivate AIEC
        Bob ->> BIECCB:OnCorrected

        end loop

        deactivate Bob
        deactivate Alice
    @enduml


### Privacy Amplification inter-communication
Privacy amplification data is passed to the other side using the IPrivacyAmplify interface.
The results are published using the IKeyCallback interface.

    @startuml PrivacyCorrection
        hide footbox
        title Privacy Correction

        control "Bob"
        boundary "Alice:IPrivacyAmplify" as AIPA [[d9/d97/classcqp_1_1_i_privacy_amplify.html]]
        boundary "Bob:IKeyCallback" as BIKCB [[d1/db5/classcqp_1_1_i_key_callback.html]]
        boundary "Alice:IKeyCallback" as AIKCB [[d1/db5/classcqp_1_1_i_key_callback.html]]

        activate Bob

        loop

        Bob -> Bob:WaitForData
        Bob -> AIPA:AmplifyPrivacy
        activate AIPA
        AIPA ->> AIKCB:OnKeyGeneration
        Bob <-- AIPA:Results
        deactivate AIPA
        Bob ->> BIKCB:OnKeyGeneration

        end loop

        deactivate Bob
    @enduml
