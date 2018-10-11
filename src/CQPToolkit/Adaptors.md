# Network Interface Adaptors

This collection of classes provides network adaptors for the interfaces. This seperates the algorithmic code from remote communication/rpc.
The header Interface/IRemoting.h defienes the interfaces cqp::IRemoting and cqp::IRemotingCallback for implementing remote procedure calls. The calss cqp::net::Remoting is an implementation which uses tcp/ip sockets and JSON strings to perform the rpc calls.
