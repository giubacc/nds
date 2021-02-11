# Naive Distributed Storage

Naive Distributed Storage (NDS in brief) is a C/C++ software system that can share data across a local network (LAN).  
NDS nodes can be spawned in the LAN on any host and without limitation to the number of instances running on the same host.  
Such NDS cluster can retain a value - a string of any size - as long as at least 1 daemon node keeps alive.  
A NDS node can either act as daemon or act as a client getting/setting the value hold by the cluster.

## Operational Requirements

NDS network protocol requires that UDP multicast traffic is enabled over the LAN.  
NDS network protocol strongly requires hosts clocks to be synched.  
Currently, TTL of UDP packets sent by a NDS node is hardcoded to 2. 

## Building Requirements

NDS is written in modern C/C++ and you will require at least a GNU g++ C++11 compliant compiler in order to build the executable.  
Used Docker image provides gcc version 8.3.0.
The project repository is CI enabled and can build itself an NDS executable.  
Refer to .gitlab-ci.yml file for more informations.  
You can manually build the executable executing:

```
./checkout_contribs.sh
make
```

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

## Network Protocol

Network Protocol used by NDS relies on both UDP/IP multicast and TCP/IP point 2 point communications.  
In nutshell, alive/status/DNS messages are all sent over multicast group; value (data) related messages are sent point 2 point via TCP/IP.  
The idea behind this is that coordination traffic, hopefully lightweight, goes through multicast, and value traffic, potentially much more heavy, goes over a unicast communication.  
The protocol heavly relies on the lastest timestamp (TS) produced by the cluster. 
All the messages, both alive (UDP) and data (TCP), are encapsulated in Json format.
All network level packets start with 4 bytes denoting the length of the subsequent payload (that is the Json body).

### How the synchronization process works

- Nodes own both a `current TS` and `desired TS`, if these 2 values differ a node try to reach a state where the `current TS` matches the `desired TS`.
- When a node spawns up, it first send an alive message in the multicast group with a TS set to zero.
- A existing node receiving an alive message checks the TS of the reveived message against its current/desired TS.  
Various scenarios can happen here:

    - (`current TS` == `foreign TS`)  
    This node is synched with foreign node, do nothing. 

    - (`current TS` > `foreign TS`) && (`current TS` == `desired TS`)  
    This node has an updated value with respect to the foreign node.  
    This node sends an alive message.

    - (`current TS` > `foreign TS`) && (`current TS` != `desired TS`)  
    This node has an updated value with respect to the foreign node but this node is synching too.  
    Do nothing.

    - (`current TS` < `foreign TS`) && (`desired TS` < `foreign TS`)  
    This node has a previous value with respect to the foreign node.  
    Update desired TS with foreign TS and request direct TCP/IP connection to the foreign node.

    - (`current TS` < `foreign TS`) && (`desired TS` >= `foreign TS`)  
    This node has a previous value with respect to the foreign node but this node is synching too.  
    Do nothing.

## Software Architecture

NDS executable consist of 2 threads communicating each other:

1. selector thread
2. peer thread

### Selector thread

Selector thread is responsible of low level communication with host interfaces and sockets.
All UDP/TCP connections are all monitored for read/write events.
When a reading event is available, the selector thread read from the socket, pack-up the body into a message and send it to the peer thread through a queue.
Selector thread is driven by the peer thread, it has no applicative logic, it only exist to serve the peer thread requests and to notify it when new network events occurr.

### Peer thread

Peer thread is the brain of the application; it knows how to interpret messages coming from Selector thread and how to trigger appropriate actions.

## Third party libraries employed

Third party libraries are imported in the project as git submodules.  
When CI runs there is a script that checkouts the appropriate tag in the submodule.
External libraries are:

- [clipp](https://github.com/muellan/clipp)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- [spdlog](https://github.com/gabime/spdlog)

## Further documentation

Please refer to code comments for an in depth explanation of the functioning of the system.

