# TLS and QKD

## The protocol

The TLS standard has the ability to use QKD, but it requires some tweaks to the applications and how they negotiate the link.

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

    cl -> srv : ClientHello(TLS_PSK_WITH_AES_256_GCM_SHA_384)
    activate srv
    cl <-- srv : ServerHello(TLS_PSK, IdentityHint)
    deactivate srv
    cl -> srv : ClientKeyExchange(\nIdentity=pkcs:object=hsm.isp.net;id=5678)
    note over ksa, srv
        The object field shows the key exchange endpoint 
        where the client got it's key. The servers key 
        store will have a key between it and hsm.isp.net.
    end note

    activate srv
    srv -> ksb : GetKey("hsm.isp.net", 5678)
    activate ksb
    srv <-- ksb : KeyValue
    deactivate ksb
    deactivate srv

    cl --> srv : ChangeCipherSpec
    cl --> srv : Finished
    activate srv
    cl <-- srv : Finished
    deactivate srv

    == Application Data ==

@enduml

The toolkit can store generated key into a HSM or similar device using the PKCS#11 standard. The `cqp::keygen::HSMStore` class uses the `cqp::p11` classes to access and compatible device such as smart cards, etc. It stores the keys with the label set to the address of the other keystore, this is passed to the other side when it is used. This means that when the key is requested it can be extracted at the other end from their local keystore.

> Note: A servers list of supported ciphers can be show with `nmap -sV --script ssl-enum-ciphers -p <port> <host>`

## OpenSSL

[OpenSSL](https://www.openssl.org/) is widely used, mainly in server applications. It supports nearly all standards and ciphers.
The OpenSSLHandler_ServerCallback and OpenSSLHandler_ClientCallback methods link the OpenSSL PSK callbacks, used to get a key when required. In fact, OpenSSL will not enable the PSK ciphers until these callbacks are registered.
OpenSSL does have support for PKCS#11, however there is no connection between the engine mechanism that it uses and the PSK cipher algorithms, for the purposes of PSK, the PKCS#11 engine is useless.

## BoringSSL

Dues to fustrations with OpenSSL, google produced a fork call [BoringSSL](https://opensource.google.com/projects/boringssl). It does not include all the features of OpenSSL but it does support PSK.

## NSS

[NSS](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSS) is the security services behind Mozilla products such as [Firefox](https://www.mozilla.org/en-US/firefox/new/). It doesn't support PSK. It does support PKCS#11, however there is no facility to search for keys other that by their id so cannot be directly used by this scheme.

# HSMs

## SoftHSM2

TODO

## YubiHSM2

The yubihsm-connector program needs to be running for the pkcs11 tool to work.
Binaries are provided for Ubuntu but as yet there is no source available.
The default password is 0001password, the 0001 is the authtoken.

> **Arch Linux**
>
> Use `debtap` to convert the ubuntu 18.04 release to pacman packages.
> Alter the dependencies for the setup package to suit.

- Run `yubihsm-connector -d`
- Create the config file `yubihsm_pkcs11.conf` with the line: `connector = http://127.0.0.1:12345`


### Issues

- The key ID is limited to 2 bytes!
- the only meta data for a key is it's ID and label even through their api.
