@0xee142a5fcd58026d;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("dht");

struct Node {
  id @0 :Data;
  ip @1 :Text;
  port @2 :UInt16;
}

interface Peer {
  getSuccessor @0 (key :Data) -> (peerInfo :Node);
}