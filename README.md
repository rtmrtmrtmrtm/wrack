# wrack

A toy that compiles a vote-count schema into C++, then
executes a 100% write vote benchmark.

make
./votebm

On my machine this yields about 7.5 million writes/second. Maybe
the closest comparison is with the paper's DBToaster measurement,
which reports less than a million writes per second.

The schema is in votebm-schema.txt. The C++ code it compiles to
appears in votebm-schema.cc. The benchmark driver is votebm.cc.

However, virtually every feature of Noria is missing from this
toy, including:

  * RPC interface (the client code is directly linked)

  * persistence

  * negative records

  * schema change

  * partial

  * multi-core
