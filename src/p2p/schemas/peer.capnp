@0xa183a1398caa893f;

interface Peer {
  struct PeerInfo {
    key @0 :Data;
    ip @1 :Text;
    port @2 :UInt16;
  }
  getSuccessor @0 (key :Data) -> (peerInfo :PeerInfo);
}
