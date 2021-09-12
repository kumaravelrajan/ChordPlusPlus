This is the final report for DHT-4 project.

[[_TOC_]]

# Overview

## What is a Distributed hash table (DHT)?
A distributed hash table (DHT) is a distributed storage for key-value pairs. An implementation of a DHT forms a structured overlay to optimize the placement and retrieval of the key-value pairs. Certain well established designs for DHT are the Chord, Pastry, Kademlia, and GNUnet’s R5N. Our implementation used the Chord algorithm. 

A DHT implementation supports storing a key-value pair and retrieval of the value given the corresponding key at any later point. The key-value pair can expire after a predefined timeout (TTL - Time to live). In our implementation, the keys are all 256 bit in length in accordance with the specification. To account for churn our implementation supports replication while storing a key-value pair.

![](./f_assets/DHT.png "https://upload.wikimedia.org/wikipedia/commons/thumb/9/98/DHT_en.svg/750px-DHT_en.svg.png")*Distributed hash table*

## The Chord algorithm
Nodes and keys are assigned an $`m-bit`$ identifier using consistent hashing. The SHA-1 algorithm is the base hashing function for consistent hashing. Consistent hashing is integral to the robustness and performance of Chord because both keys and nodes (in fact, their IP addresses) are uniformly distributed in the same identifier space with a negligible possibility of collision. Thus, it also allows nodes to join and leave the network without disruption. In the protocol, the term node is used to refer to both a node itself and its identifier (ID) without ambiguity. So is the term key.

Using the Chord lookup protocol, nodes and keys are arranged in an identifier circle that has at most $`2^{m}`$ nodes, ranging from $`0`$ to $`2^m - 1`$. $`m`$ should be large enough to avoid collision.) Some of these nodes will map to machines or keys while others (most) will be empty.

Each node has a successor and a predecessor. The successor to a node is the next node in the identifier circle in a clockwise direction. The predecessor is counter-clockwise. If there is a node for each possible ID, the successor of node 0 is node 1, and the predecessor of node 0 is node $`2^m - 1`$; however, normally there are "holes" in the sequence. For example, the successor of node 153 may be node 167 (and nodes from 154 to 166 do not exist); in this case, the predecessor of node 167 will be node 153.

The concept of successor can be used for keys as well. The successor node of a key $`k`$ is the first node whose ID equals to $`k`$ or follows $`k`$ in the identifier circle, denoted by $`successor(k)`$. Every key is assigned to (stored at) its successor node, so looking up a key $`k`$ is to query $`successor(k)`$.

Since the successor (or predecessor) of a node may disappear from the network (because of failure or departure), each node records a whole segment of the circle adjacent to it, i.e., the $`r`$ nodes preceding it and the $`r`$ nodes following it. This list results in a high probability that a node is able to correctly locate its successor or predecessor, even if the network in question suffers from a high failure rate.

![](./f_assets/ChordOverview.png "https://upload.wikimedia.org/wikipedia/commons/thumb/1/16/Chord_project.svg/525px-Chord_project.svg.png")*Overview of chord algorithm*

### Protocol details 
![](./f_assets/Chord_network.png "https://upload.wikimedia.org/wikipedia/commons/thumb/2/20/Chord_network.png/375px-Chord_network.png")<br>*The "fingers" for one of the nodes are highlighted in a 16-node ring.*

#### Basic query
The core usage of the Chord protocol is to query a key from a client (generally a node as well), i.e. to find $`successor(k)`$. The basic approach is to pass the query to a node's successor, if it cannot find the key locally. This will lead to a $`O(N)`$ query time where $`N`$ is the number of machines in the ring.

#### Finger table
To avoid the linear search above, Chord implements a faster search method by requiring each node to keep a finger table containing up to $`m`$ entries($`m`$ being the number of bits in the hash key). The $`i^{th}`$ entry of node $`n`$ will contain $`successor((n+2^{i-1}) mod 2^m)`$. The first entry of finger table is actually the node's immediate successor (and therefore an extra successor field is not needed). Every time a node wants to look up a key $`k`$, it will pass the query to the closest successor or predecessor (depending on the finger table) of $`k`$ in its finger table (the "largest" one on the circle whose ID is smaller than $`k`$), until a node finds out the key is stored in its immediate successor.

With such a finger table, the number of nodes that must be contacted to find a successor in an N-node network is $`O(log N)`$.

#### Node join
Whenever a new node joins, three invariants should be maintained (the first two ensure correctness and the last one keeps querying fast):

1. Each node's successor points to its immediate successor correctly.
1. Each key is stored in $`successor(k)`$.
1. Each node's finger table should be correct.

To satisfy these invariants, a predecessor field is maintained for each node. As the successor is the first entry of the finger table, we do not need to maintain this field separately any more. The following tasks are done for a newly joined node $`n`$:

1. Initialize node $`n`$ (the predecessor and the finger table).
1. Notify other nodes to update their predecessors and finger tables.
1. The new node takes over its responsible keys from its successor.

The predecessor of $`n`$ can be easily obtained from the predecessor of $`successor(n)`$ (in the previous circle). As for its finger table, there are various initialization methods. The simplest one is to execute find successor queries for all $`m`$ entries, resulting in $`O(Mlog N)`$ initialization time. A better method is to check whether $`i^{th}`$ entry in the finger table is still correct for the $`(i+1)^{th}`$ entry. This will lead to $`O(log^2 N)`$. The best method is to initialize the finger table from its immediate neighbours and make some updates, which is $`O(\log N)`$.

#### Stabilization
To ensure correct lookups, all successor pointers must be up to date. Therefore, a stabilization protocol is running periodically in the background which updates finger tables and successor pointers.

The stabilization protocol works as follows:

1. Stabilize(): n asks its successor for its predecessor p and decides whether p should be n‘s successor instead (this is the case if p recently joined the system).
1. Notify(): notifies n‘s successor of its existence, so it can change its predecessor to n
1. Fix_fingers(): updates finger tables

#### Pseudocode
##### Definitions for pseudocode
1. finger[k] : first node that succeeds $`(n+2^{k-1}) \enspace mod \enspace 2^m, 1 \leq k \leq m`$
1. successor : the next node from the node in question on the identifier ring
1. predecessor : the previous node from the node in question on the identifier ring

##### 1. Find the successor node of an id
```
// ask node n to find the successor of id
n.find_successor(id)
    // Yes, that should be a closing square bracket to match the opening parenthesis.
    // It is a half closed interval.
    if id ∈ (n, successor] then
        return successor
    else
        // forward the query around the circle
        n0 := closest_preceding_node(id)
        return n0.find_successor(id)

// search the local table for the highest predecessor of id
n.closest_preceding_node(id)
    for i = m downto 1 do
        if (finger[i] ∈ (n, id)) then
            return finger[i]
    return n
```

##### 2. Stabilize the chord ring/circle after node joins and departures
```
// create a new Chord ring.
n.create()
    predecessor := nil
    successor := n

// join a Chord ring containing node n'.
n.join(n')
    predecessor := nil
    successor := n'.find_successor(n)

// called periodically. n asks the successor
// about its predecessor, verifies if n's immediate
// successor is consistent, and tells the successor about n
n.stabilize()
    x = successor.predecessor
    if x ∈ (n, successor) then
        successor := x
    successor.notify(n)

// n' thinks it might be our predecessor.
n.notify(n')
    if predecessor is nil or n'∈(predecessor, n) then
        predecessor := n'

// called periodically. refreshes finger table entries.
// next stores the index of the finger to fix
n.fix_fingers()
    next := next + 1
    if next > m then
        next := 1
    finger[next] := find_successor(n+{\displaystyle 2^{next-1}}2^{next-1});

// called periodically. checks whether predecessor has failed.
n.check_predecessor()
    if predecessor has failed then
        predecessor := nil
```


## The implementation
