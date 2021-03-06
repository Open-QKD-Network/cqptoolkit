---
title: 'QComms QKD Software Toolkit'
tags:
  - c++
  - qkd
  - physics
  - security
  - quantum
authors:
  - name: Mr Richard Collins
    orcid: 0000-0002-6238-990X
    affiliation: 1
  - name: Djeylan Aktas
    orcid: 0000-0002-5747-0586
    affiliation: 1
affiliations:
  - name: University of Bristol
    index: 1
date: 11 October 2018
bibliography: paper.bib
nocite: |
  @
---

# Summary

The purpose of this software is to provide a set of interfaces, algorithms and tools to produce a full QKD system. The aim is to reduce the barrier to entry into QKD systems by developing a strong architecture with well defined interfaces. The separation of responsibility into components will allow researchers to concentrate on a specific component, the rest of the system being provided by existing algorithms or simulators.

Quantum key distribution (QKD) uses properties of quantum states such as the [no-cloning theorem](https://www.quantiki.org/wiki/no-cloning-theorem) to share randomness between different locations, without the need to physically transport the key material. This randomness is known to be secure because the probability of an eavesdropper can be calculated. This means that [symmetric encryption](https://www.ssl2buy.com/wiki/symmetric-vs-asymmetric-encryption-what-are-differences) instead of public key can be used, which can prevent attacks from [quantum computers](https://www.cryptomathic.com/news-events/blog/quantum-computing-and-its-impact-on-cryptography). Symmetric keys are only as secure as the medium they are transported over. Unlike QKD, manually exchanged key is expensive (e.g. physically driving somewhere with a flash drive) and has to be used sparingly.  Making guaranteed secure key available as a resource makes symmetric keys more viable.

The system demonstrates that the symmetric keys can be used in a disposable manner rather than the current model where valuable keys are derived to extend their use.
There are many utilities in place to simplify the use of the toolkit such as statistics collection for gathering data.

As well as optical device research, the system can be used to develop key management for networks.

![Toolkit Stack](ToolkitStack.eps)

# Components
## Statistics
The statistics component provides a means for publishing data from within an algorithm without adversely affecting the system's performance and allowing the data to be collected through a simple external interface.

## Device drivers
The system currently include drivers for the IDQuantique Clavis 2 QKD devices.
To use a device with the system, a driver needs to be written to provide data to one of the entry points of the pipeline. The driver could provide raw qubits, ready to use key or anything in between. Once a driver for specific hardware is written, the standard set of post processing tools can be used to extract usable key and provide statistics on the operation of the system as a whole.

## Post Processing
Processing of Qubits is split into 4 sections:

- Alignment of receiver with transmitter, which involve a hardware check-up loop (laser power, noise measurement...) and a line length measurement loop.
- Sifting procedure (plug & play BB84 [@QKDprotocol]) that chose the proper states after an optical setting loop of phase adjustments and visibility measurements.
- Error correction of detected bits.
- Privacy amplification to obtain the final distilled key.

Once the key is produced by the post processing it is passed to the key store. Some of the algorithms have been adapted from David Lowndes paper [@HandHeld].

## Key Management
There are some issues unique to shared secrets which the current public key systems do not provide for, the system tackles some of these issues by providing a standardised interface to the keys. The key store classes hold keys for specific links between two sites, while session management starts and stops the key generation.

## Site Management
Each collection of key stores constitute trusted nodes of the network, Site agents manage the connections with other sites and feature several functions [@SDNpaper] that can be integrated with software defined networks for dynamic key generation [@SDNpaper2].

# Acknowledgements
This project has been funded by [EPSRC](https://epsrc.ukri.org/) under the [Quantum communications hub](https://www.quantumcommshub.net/).
We acknowledge contributions from Dr Djeylan Aktas, Dr David Lowndes, Prof. John Rarity, Dr Chris Erven at the University of Bristol.

# References

