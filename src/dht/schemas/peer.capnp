@0xee142a5fcd58026d;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("dht");

struct Optional(T) {
  union {
    value @0 :T;
    empty @1 :Void;
  }
}

struct Node {
  id   @0 :Data;
  ip   @1 :Text;
  port @2 :UInt16;
}

struct DataItem {
  key     @0 :Data;
  data    @1 :Data;
  expires @2 :UInt64;
}

interface Peer {
  getSuccessor       @0 (id :Data)      -> (node :Optional(Node));
  getPredecessor     @1 ()              -> (node :Optional(Node));
  notify             @2 (node :Node);
  getData            @3 (key :Data)     -> (data :Optional(Data));
  setData            @4 (key :Data, value :Data, ttl :UInt16 = 0);
  getDataItemsOnJoin @5 (newNode :Node) -> (listOfDataItems :List(DataItem));
  getPoWPuzzleOnJoin @6 (newNode :Node) -> (proofOfWorkPuzzle :Text, difficulty :UInt16);
  sendPoWPuzzleResponseToBootstrapAndGetSuccessor @7 (newNode :Node, proofOfWorkPuzzleResponse :Text, hashOfproofOfWorkPuzzleResponse :Text) -> (successorOfNewNode :Optional(Node));
}