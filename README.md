# Naive Distributed Storage

Naive Distributed Storage (NDS in brief) is a C/C++ software system that can share data across a local network (LAN).  
NDS nodes can be spawned in the LAN on any host and without limitation* to the number of instances running on the same host.  
Such NDS mesh can retain a value - a string of any size - as long as at least 1 daemon node remains alive.  
A NDS node can either act as daemon (pure node) or act as a client getting or setting the value hold by the mesh.

## Operational Requirements

NDS network protocol requires that UDP multicast traffic is enabled over the LAN.
Currently, TTL of UDP packets sent by a NDS node is hardcoded to 2. 

## Building Requirements

NDS is written in modern C/C++ and you will require at least a GNU g++ C++11 compliant compiler in order to build the executable.
Used Docker image provides gcc version 8.3.0.

## Usage

