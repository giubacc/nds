# Naive Distributed Storage

Naive Distributed Storage (NDS in brief) is a C/C++ software system that can share data across a local network (LAN).
NDS nodes can be spawned inside the LAN on any host and without limitation* to the number of instances running on the same host.
Such NDS mesh can retain a value - a string of any size - as long as at least 1 daemon node remains alive. 
A NDS node can either act as daemon (pure node) or act as a client getting or setting the value on the mesh.


