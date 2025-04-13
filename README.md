![Header](images/header.png)

# Introduction
A Distributed Hash Table (DHT) is a system used to store and retrieve data in a decentralized manner. In a DHT, data is stored as key-value pairs. Instead of storing all the data in one place, the keys and their corresponding values are distributed among many nodes. Each node is responsible for managing a portion of the data. 

When one wants to store or retrieve data, the system uses the key to determine which node is responsible for it. This is done using a hashing function, ensuring efficient and quick lookups. DHTs can handle millions of nodes, as they are designed to scale efficiently. 

# Chord algorithm
The Chord algorithm is a protocol for implementing a Distributed Hash Table (DHT) that organizes nodes and data in a decentralized, ring-shaped network. It enables efficient key lookups with minimal overhead, even as nodes dynamically join or leave the system.

This project implements the chord algorithm and in addition to improve the security of the P2P network implements measures such as symmetric encryption and proof-of-work. 

# Setup

Ensure docker is installed on your system. Run the dockerfile.

