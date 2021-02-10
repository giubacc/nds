# Naive Distributed Storage

Naive Distributed Storage (NDS in brief) is a C/C++ software system that can share data across a local network (LAN).  
NDS nodes can be spawned in the LAN on any host and without limitation* to the number of instances running on the same host.  
Such NDS mesh can retain a value - a string of any size - as long as at least 1 daemon node remains alive.  
A NDS node can either act as daemon (pure node) or act as a client getting or setting the value hold by the mesh.

## Operational Requirements

NDS network protocol requires that UDP multicast traffic is enabled over the LAN.  
NDS network protocol strongly requires hosts clocks to be synched.  
Currently, TTL of UDP packets sent by a NDS node is hardcoded to 2. 

## Building Requirements

NDS is written in modern C/C++ and you will require at least a GNU g++ C++11 compliant compiler in order to build the executable.  
Used Docker image provides gcc version 8.3.0.

## Usage

```
SYNOPSIS
        ./nds [-n] [-j <multicast address>] [-p <listening port>] [-l <logging type>] [-v <logging verbosity>] [set <value>] [get]

OPTIONS
        -n, --node  spawn a new node
        -j, --join  join the cluster at specified multicast group
        -p, --port  listen on the specified port
        -l, --log   specify logging type [console (default), file name]
        -v, --verbosity
                    specify logging verbosity [off, trace, info (default), warn, err]

        set         set the value shared across the cluster
        get         get the value shared across the cluster
```

#### Examples

`nds` try to get the value from the cluster (if exists), if a value can be obtained the program will print it on stdout and then it will exit.    
`nds -n` spawns a new daemon node in the cluster using default UDP multicast group (`232.232.200.82:8745`).  
`nds -n -v trace set Jerico` spawns a new daemon node and contestually set value `Jerico` in the cluster (also console log verbosity is set to trace).  
`nds -n -j 232.232.211.56 -p 26543` spawns a new daemon node using provided UDP multicast group and the listening TCP port.


## Synching Protocol

Synching Protocol used by NDS relies on both UDP/IP multicast and TCP/IP point 2 point communications.  
In nutshell, alive/status/DNS messages are all sent over multicast group; value (data) related messages are sent point 2 point via TCP/IP.  
The idea behind this is that coordination traffic, hopefully lightweight, goes through multicast, and value traffic, potentially much more heavy, goes over a unicast communication.  



